
#include "spp_connection.h"
#include "spp_server.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>

#include <ev.h>

#include <log4cxx/logger.h>

///////////////////////////////////////////////////////////////////////////////
// macros
///////////////////////////////////////////////////////////////////////////////

#define MAC_ADDR_STRING_LENGTH ((2*6)+5)

///////////////////////////////////////////////////////////////////////////////
// type defintions
///////////////////////////////////////////////////////////////////////////////

struct connection_io
{
    struct ev_io io;
    APPManager::Message *message_p;
    bool close;
};

///////////////////////////////////////////////////////////////////////////////
// constants
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// module variables
///////////////////////////////////////////////////////////////////////////////

static log4cxx::LoggerPtr g_logger(log4cxx::Logger::getLogger("bluetooth.spp.connection"));

///////////////////////////////////////////////////////////////////////////////
// private function declarations
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// public function implementations
///////////////////////////////////////////////////////////////////////////////

SPPConnection::SPPConnection(SPPServer *server_p, int socket, const bdaddr_t *remote_addr_p) :
    m_server_p(server_p),
    m_socket(socket),
    m_loop_p(ev_default_loop(0)),
    m_handler_p(APPManager::createSPPConnector(this))
{
    LOG4CXX_DEBUG(g_logger, "SPPConnection::SPPConnection enter " << server_p << " " << socket);

    // store the remote address
    memcpy(&m_remote_addr, remote_addr_p, sizeof(bdaddr_t));

    // initialize the io watcher for the socket
    ev_io_init(&m_watcher, socket_cb, m_socket, EV_READ);
    m_watcher.data = (void *)this;

    // register the listener with the loop
    ev_io_start(m_loop_p, &m_watcher);

    LOG4CXX_DEBUG(g_logger, "SPPConnection::SPPConnection exit");
}

SPPConnection::~SPPConnection()
{
    LOG4CXX_DEBUG(g_logger, "SPPConnection::~SPPConnection enter");

    // remove the watcher
    ev_io_stop(m_loop_p, &m_watcher);
    // close the socket
    if (-1 != m_socket)
    {
        LOG4CXX_INFO(g_logger, "disconnecting connection from " << format_mac_addr(&m_remote_addr));
        close(m_socket);
        m_socket = -1;
    }
    // clear ourselves in the server
    m_server_p->m_connection_p = NULL;


    LOG4CXX_DEBUG(g_logger, "SPPConnection::~SPPConnection exit");
}

std::string SPPConnection::format_mac_addr(const bdaddr_t *addr_p)
{
    std::string buffer_s(MAC_ADDR_STRING_LENGTH, ' ');
    snprintf((char *) buffer_s.c_str(), buffer_s.capacity(),
             "%02X:%02X:%02X:%02X:%02X:%02X", addr_p->b[0], addr_p->b[1],
             addr_p->b[2], addr_p->b[3], addr_p->b[4], addr_p->b[5]);
    return buffer_s;
}


///////////////////////////////////////////////////////////////////////////////
// APPManager:NotificationHandler implementation
///////////////////////////////////////////////////////////////////////////////

ResultCode SPPConnection::send_notification(APPManager::Message **notification_pp)
{
    LOG4CXX_DEBUG(g_logger, "SPPConnection::send_notification enter " << notification_pp);

    // get the message pointer
    APPManager::Message *notification_p = *notification_pp;
    // queue the message to send when the socket is ready to write
    connection_io send_watcher;
    ev_io_init(&send_watcher.io, send_cb, m_socket, EV_WRITE);
    // set the watcher data
    send_watcher.io.data = (void *)notification_p;
    send_watcher.message_p = notification_p;
    send_watcher.close = false;
    // send the event
    ev_io_start(m_loop_p, &send_watcher.io);
    // tell the caller we took overship of the message
    *notification_pp = NULL;

    LOG4CXX_DEBUG(g_logger, "SPPConnection::send_notification exit");

    return RESULT_CODE_OK;
}

///////////////////////////////////////////////////////////////////////////////
// private function implementations
///////////////////////////////////////////////////////////////////////////////

void SPPConnection::socket_cb (EV_P_ ev_io *w_p, int revents)
{
    LOG4CXX_DEBUG(g_logger, "SPPConnection::socket_cb enter " << w_p << " " << revents);

    // get the object
    SPPConnection *connection_p = (SPPConnection *)(w_p->data);

    do
    {
        // handling for when clients disconnect
        if (0 != (revents & EV_ERROR))
        {
            LOG4CXX_DEBUG(g_logger, "SPPConnection::socket_cb EV_ERROR");
            // delete the connection
            delete connection_p;
            // done
            break;
        }
        if (0 != (revents & EV_READ))
        {
            LOG4CXX_DEBUG(g_logger, "SPPConnection::socket_cb EV_READ");
            // read the number of bytes we should expect in the message
            uint16_t network_length;
            int rc = read(w_p->fd, &network_length, sizeof(network_length));
            if (0 > rc)
            {
                LOG4CXX_INFO(g_logger, "connection from " << format_mac_addr(&(connection_p->m_remote_addr)) << " closed");
                // the socket is already closed but we'll call close here and cleanup the connection object to avoid a 
                // spurious log message in the destructor
                close(connection_p->m_socket);
                connection_p->m_socket = -1;
                // cleanup the connection  
                delete connection_p;
                break;
            }
            // create the request and response message containers
            APPManager::Message request;

            // convert from network order
            request.length = ntohs(network_length);

            // allocate the amount of memory we need to hold the message
            request.data_p = (uint8_t *)malloc(request.length);

            // read the actual data
            rc = read(w_p->fd, (void *)request.data_p, request.length);
            if (0 > rc)
            {
                LOG4CXX_ERROR(g_logger, "read returned error=" + rc);
                // free the memory we just allocated
                free(request.data_p);
                // force a disconnect since we were unable to read the message
                delete connection_p;
                // done
                break;
            }

            // this is a pointer to the response message
            APPManager::Message *response_p = NULL;

            // call the handler
            ResultCode result_code = connection_p->m_handler_p->handle_request(&request, &response_p);
            if (RESULT_CODE_OK == result_code)
            {
                ASSERT(NULL != response_p);

                LOG4CXX_DEBUG(g_logger, "SPPConnection::socket_cb request handler returned ok");

                // queue the response to send when the socket is ready to write
                connection_io send_watcher;
                ev_io_init(&send_watcher.io, send_cb, w_p->fd, EV_WRITE);
                // set the user data
                send_watcher.io.data = (void *)connection_p;
                send_watcher.message_p = response_p;
                send_watcher.close = false;
                // send the event
                ev_io_start(connection_p->m_loop_p, &send_watcher.io);
            }
            else
            {
                LOG4CXX_ERROR(g_logger, "SPPConnection::socket_cb request handler returned error=" << result_code << " " << response_p);
                // returning an error tells us we should disconnect
                // however they can still ask us to send back a message before disconnecting
                if (NULL != response_p)
                {
                    // queue the message to send when the socket is ready to write
                    connection_io send_and_close_watcher;
                    ev_io_init(&send_and_close_watcher.io, send_cb, w_p->fd, EV_WRITE);
                    // set the user data
                    send_and_close_watcher.io.data = (void *)connection_p;
                    send_and_close_watcher.message_p = response_p;
                    send_and_close_watcher.close = true;
                    // send the event
                    ev_io_start(connection_p->m_loop_p, &send_and_close_watcher.io);
                }
                else
                {
                    // close now
                    delete connection_p;
                }
            }
            // free the buffer
            free(request.data_p);
        }
    }
    while (false);

    LOG4CXX_DEBUG(g_logger, "SPPConnection::socket_cb exit");
}

void SPPConnection::send_cb(EV_P_ ev_io *w_p, int revents)
{
    LOG4CXX_DEBUG(g_logger, "SPPConnection::send_cb enter " << w_p << " " << revents);

    // get the object
    SPPConnection *connection_p = (SPPConnection *)(w_p->data);

    // get the message
    APPManager::Message *message_p = ((connection_io *)w_p)->message_p;

    // this is a one-shot event
    ev_io_stop (EV_A_ w_p);

    // use a do-while to provide a simple break-out
    do
    {
        if (0 != (revents & EV_ERROR))
        {
            LOG4CXX_DEBUG(g_logger, "SPPConnection::socket_cb EV_ERROR");

            // cleanup the client
            delete connection_p;
            // done
            break;
        }
        if (0 != (revents & EV_WRITE))
        {
            LOG4CXX_DEBUG(g_logger, "SPPConnection::socket_cb EV_ERROR");

            // send the length
            uint16_t network_length = htons(message_p->length);
            int rc = write(w_p->fd, &network_length, sizeof(network_length));
            if (0 > rc)
            {
                LOG4CXX_ERROR(g_logger, "error returned from write " << rc);

                // if we can't write then the client must have disconnected
                delete connection_p;
                // done
                break;
            }
            // send the message
            if (0 > write(w_p->fd, message_p->data_p, message_p->length))
            {
                LOG4CXX_ERROR(g_logger, "error returned from write " << rc);

                // have to disconnect as we were unable to write the whole message
                delete connection_p;
                // done
                break;
            }
            // if they asked us to close the connection do so now
            if (true == ((connection_io *)w_p)->close)
            {
                LOG4CXX_WARN(g_logger, "closing connection");
                delete connection_p;
            }
        }
    }
    while (false);

    // free the memory
    free(message_p);

    LOG4CXX_DEBUG(g_logger, "SPPConnection::send_cb exit");
}

