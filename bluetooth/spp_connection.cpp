
#include "spp_connection.h"
#include "spp_server.h"
#include "log.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <ev.h>


///////////////////////////////////////////////////////////////////////////////
// macros
///////////////////////////////////////////////////////////////////////////////

#define MAC_ADDR_STRING_LENGTH ((2*6)+5+1)

///////////////////////////////////////////////////////////////////////////////
// type defintions
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// constants
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// module variables
///////////////////////////////////////////////////////////////////////////////

static LogInstance g_logger("bluetooth.spp.connection");

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
    LOG_GENERATE_TRACE(g_logger, "SPPConnection::SPPConnection enter this=%p server_p=%p socket=%d remote_addr_p=%p", 
            this, server_p, socket, remote_addr_p);

    // store the remote address
    memcpy(&m_remote_addr, remote_addr_p, sizeof(bdaddr_t));

    // turn on non-blocking io on the socket
    int on = 1;
    int rc = ioctl(socket, FIONBIO, (char *)&on);
    if (0 != rc)
    {
        LOG_GENERATE_ERROR(g_logger, "unable to set non-blocking i/o on socket=%d", socket);
        return;
    }

    // initialize the io watchers 
    ev_io_init(&m_receive_watcher, receive_cb, m_socket, EV_READ);
    m_receive_watcher.data = (void *)this;
    ev_io_init(&m_transmit_watcher, transmit_cb, m_socket, EV_WRITE);
    m_transmit_watcher.data = (void *)this;

    // only register the rx size listener with the loop
    ev_io_start(m_loop_p, &m_receive_watcher);

    LOG_GENERATE_TRACE(g_logger, "SPPConnection::SPPConnection exit");
}

SPPConnection::~SPPConnection()
{
    LOG_GENERATE_TRACE(g_logger, "SPPConnection::~SPPConnection enter this=%p", this);

    // remove the watchers
    ev_io_stop(m_loop_p, &m_receive_watcher);
    ev_io_stop(m_loop_p, &m_transmit_watcher);

    // close the socket
    if (-1 != m_socket)
    {
        LOG_GENERATE_INFO(g_logger, "disconnecting from %s", format_mac_addr(&m_remote_addr).c_str());
        close(m_socket);
        m_socket = -1;
    }

    // delete the handler
    delete m_handler_p;

    // clear ourselves in the server
    m_server_p->m_connection_p = NULL;

    LOG_GENERATE_TRACE(g_logger, "SPPConnection::~SPPConnection exit");
}

const std::string SPPConnection::format_mac_addr(const bdaddr_t *addr_p)
{
    char buffer[MAC_ADDR_STRING_LENGTH + 1];
    snprintf(buffer, MAC_ADDR_STRING_LENGTH, 
             "%02X:%02X:%02X:%02X:%02X:%02X", addr_p->b[0], addr_p->b[1],
             addr_p->b[2], addr_p->b[3], addr_p->b[4], addr_p->b[5]);
    return buffer;
}

SPPConnection::Buffer::Buffer() :
    m_message_p(NULL),
    m_remaining(0)
{
    LOG_GENERATE_TRACE(g_logger, "SPPConnection::Buffer::Buffer enter this=%p", this);
    LOG_GENERATE_TRACE(g_logger, "SPPConnection::Buffer::Buffer exit");
}

SPPConnection::Buffer::~Buffer()
{
    LOG_GENERATE_TRACE(g_logger, "SPPConnection::Buffer::~Buffer enter this=%p", this);
    if (NULL != m_message_p)
    {
        delete m_message_p;
    }
    LOG_GENERATE_TRACE(g_logger, "SPPConnection::Buffer::~Buffer exit");
}

void SPPConnection::Buffer::clear()
{
   LOG_GENERATE_TRACE(g_logger, "SPPConnection::Buffer::clear enter this=%p", this);
   if (NULL != m_message_p)
   {
       delete m_message_p;
       m_message_p = NULL;
       m_remaining = 0;
   }
   LOG_GENERATE_TRACE(g_logger, "SPPConnection::Buffer::clear exit");
}

size_t SPPConnection::Buffer::calculate_offset()
{
   LOG_GENERATE_TRACE(g_logger, "SPPConnection::Buffer::Buffer enter this=%p", this);
   ASSERT(NULL != m_message_p);
   size_t offset = (m_message_p->get_length() - m_remaining);
   LOG_GENERATE_TRACE(g_logger, "SPPConnection::Buffer::Buffer exit offset=%d", offset);
   return offset;
}

void SPPConnection::Buffer::set_message(APPManager::Message *message_p)
{
    LOG_GENERATE_TRACE(g_logger, "SPPConnection::Buffer::set_message enter this=%p message_p=%p", this, message_p);
    ASSERT(NULL != message_p);
    m_message_p = message_p;
    m_remaining = message_p->get_length();
    LOG_GENERATE_TRACE(g_logger, "SPPConnection::Buffer::set_message exit");
}
///////////////////////////////////////////////////////////////////////////////
// APPManager:NotificationHandler implementation
///////////////////////////////////////////////////////////////////////////////

ResultCode SPPConnection::send_notification(APPManager::Message **notification_pp)
{
    LOG_GENERATE_TRACE(g_logger, "SPPConnection::send_notification enter this=%p notification_pp=%p", this, notification_pp);

    // queue the message to send when the socket is ready to write
    m_transmit_queue.push(*notification_pp);

    // trigger the write callback 
    ev_io_start(m_loop_p, &m_transmit_watcher);

    // tell the caller we took overship of the message
    *notification_pp = NULL;

    LOG_GENERATE_TRACE(g_logger, "SPPConnection::send_notification exit");

    return RESULT_CODE_OK;
}

///////////////////////////////////////////////////////////////////////////////
// private function implementations
///////////////////////////////////////////////////////////////////////////////

void SPPConnection::receive_cb (EV_P_ ev_io *w_p, int revents)
{
    LOG_GENERATE_TRACE(g_logger, "SPPConnection::receive_cb enter w_p=%p revents=0x%x", w_p, revents);

    do
    {
        // get the object
        SPPConnection *connection_p = (SPPConnection *)(w_p->data);

        // get the receive buffer
        Buffer *receive_p = &(connection_p->m_receive);

        // handling for when clients disconnect
        if (0 != (revents & EV_ERROR))
        {
            LOG_GENERATE_DEBUG(g_logger, "SPPConnection::receive_cb EV_ERROR");
            // disconnect
            disconnect(&connection_p);
            // done
            break;
        }
        if (0 != (revents & EV_READ))
        {
            LOG_GENERATE_TRACE(g_logger, "SPPConnection::receive_cb EV_READ");

            // see if we need to read the size
            if (true == receive_p->is_empty())
            {
                // read the number of bytes we should expect in the message
                uint16_t network_length;
                int rc = read(w_p->fd, &network_length, sizeof(network_length));
                if (0 > rc)
                {
                    LOG_GENERATE_ERROR(g_logger, "disconnecting for reason=%s", strerror(errno));
                    // disconnect
                    disconnect(&connection_p);
                    break;
                }
            
                LOG_GENERATE_DEBUG(g_logger, "read length=%d", ntohs(network_length));

                // setup the receive buffer params
                APPManager::Message *message_p = new APPManager::Message(ntohs(network_length));
                // store the message in the buffer
                receive_p->set_message(message_p);
            }
            else
            {
                // calculate the offset
                const size_t offset = receive_p->calculate_offset();

                // read the actual data
                int rc = read(w_p->fd, (void *)(receive_p->get_message_p()->get_data_p() + offset), receive_p->get_remaining_r());
                if (0 > rc)
                {
                    LOG_GENERATE_ERROR(g_logger, "disconnecting for reason=%s", strerror(errno));
                    // force a disconnect since we were unable to read the message
                    disconnect(&connection_p);
                    // done
                    break;
                }

                LOG_GENERATE_DEBUG(g_logger, "read bytes=%d data=%s", rc, to_string(receive_p->get_message_p()->get_data_p() + offset, rc).c_str());

                // update the remaining data
                receive_p->get_remaining_r() -= rc;

                // see if we've read the whole message
                if (0 == receive_p->get_remaining_r())
                {
                    LOG_GENERATE_INFO(g_logger, "received message (%d bytes) from %s", receive_p->get_message_p()->get_length(), format_mac_addr(&connection_p->m_remote_addr).c_str());

                    // this is a pointer to the response message
                    APPManager::Message *response_p = NULL;

                    // call the handler
                    ResultCode result_code = connection_p->m_handler_p->handle_request(receive_p->get_message_p(), &response_p);
                    if (RESULT_CODE_OK == result_code)
                    {
                        ASSERT(NULL != response_p);

                        LOG_GENERATE_DEBUG(g_logger, "SPPConnection::rx_message_cb request handler returned ok");

                        // use the notification send method to queue the message
                        connection_p->send_notification(&response_p);
                    }
                    else
                    {
                        LOG_GENERATE_ERROR(g_logger, "SPPConnection::rx_message_cb request handler returned error=rc response_p=%p", result_code, response_p);
                        // send back the response if provided
                        if (NULL != response_p)
                        {
                            // use the notification send method to queue the message
                            connection_p->send_notification(&response_p);
                        }
                        else 
                        {
                            // disconnect
                            disconnect(&connection_p);
                        }
                    }
                    // cleanup the receive buffer
                    receive_p->clear();
                }
            }
        }
    }
    while (false);

    LOG_GENERATE_TRACE(g_logger, "SPPConnection::receive_cb exit");
}

void SPPConnection::transmit_cb(EV_P_ ev_io *w_p, int revents)
{
    LOG_GENERATE_TRACE(g_logger, "SPPConnection::transmit_cb enter w_p=%p revents=0x%x", w_p, revents);

    // use a do-while to provide a simple break-out
    do
    {
        // get the object
        SPPConnection *connection_p = (SPPConnection *)(w_p->data);

        // get the transmit buffer
        Buffer *transmit_p = &(connection_p->m_transmit);

        if (0 != (revents & EV_ERROR))
        {
            LOG_GENERATE_TRACE(g_logger, "SPPConnection::transmit_cb EV_ERROR");
            // disconnect
            disconnect(&connection_p);
            // done
            break;
        }
        if (0 != (revents & EV_WRITE))
        {
            LOG_GENERATE_DEBUG(g_logger, "SPPConnection::socket_cb EV_WRITE");
            
            // see if we are already transmitting
            if (true == transmit_p->is_empty())
            {
                // if the transmit queue is empty we can disable ourselves
                if (true == connection_p->m_transmit_queue.empty())
                {
                    LOG_GENERATE_DEBUG(g_logger, "SPPConnection::transmit_cb queue empty");
                    // stop this callback from firing again
			        ev_io_stop (EV_A_ w_p);
                    // done
                    break;
                }

                // retrieve + remove from queue
                APPManager::Message *message_p = connection_p->m_transmit_queue.front();
                connection_p->m_transmit_queue.pop();

                // store in the buffer
                transmit_p->set_message(message_p);

				// send the length
				uint16_t network_length = htons(message_p->get_length());
				int rc = write(w_p->fd, &network_length, sizeof(network_length));
				if (0 > rc)
				{
                    LOG_GENERATE_ERROR(g_logger, "disconnecting for reason=%s", strerror(errno));
					// if we can't write then the client must have disconnected
					disconnect(&connection_p);
					// done
					break;
				}
            }
            else
            {
                // calculate the offset
                const size_t offset = transmit_p->calculate_offset();

				// send the message
				int rc = write(w_p->fd, transmit_p->get_message_p()->get_data_p() + offset, transmit_p->get_remaining_r());
                if (0 > rc)
				{
                    LOG_GENERATE_ERROR(g_logger, "disconnecting for reason=%s", strerror(errno));
					// have to disconnect as we were unable to write the whole message
					disconnect(&connection_p);
					// done
					break;
				}

                LOG_GENERATE_DEBUG(g_logger, "wrote bytes=%d data=%s", rc, to_string(transmit_p->get_message_p()->get_data_p() + offset, rc).c_str());

                // update the remaining data
                transmit_p->get_remaining_r() -= rc;

                // see if we've wrote the whole message
                if (0 == transmit_p->get_remaining_r())
                {
                    // clean up the transmit buffer
                    transmit_p->clear();

                    // if there no messages pending then we can stop the event from firing
                    if (true == connection_p->m_transmit_queue.empty())
                    {
                        LOG_GENERATE_DEBUG(g_logger, "SPPConnection::transmit_cb queue empty after transmit");
                        // stop this callback from firing again
			            ev_io_stop (EV_A_ w_p);
                    }
                }
            }
        }
    }
    while (false);

    LOG_GENERATE_TRACE(g_logger, "SPPConnection::transmit_cb exit");
}

void SPPConnection::disconnect(SPPConnection **connection_pp)
{
	LOG_GENERATE_TRACE(g_logger, "SPPConnection::disconnect enter connection_pp=%p", connection_pp);

	// get the object itself
	SPPConnection *connection_p = *connection_pp;

    LOG_GENERATE_INFO(g_logger, "connection from %s closed", format_mac_addr(&connection_p->m_remote_addr).c_str());
    // the socket is already closed but we'll call close here and cleanup the connection object to avoid a 
    // spurious log message in the destructor
    close(connection_p->m_socket);
    connection_p->m_socket = -1;

    // clear the pointer
    *connection_pp = NULL;

    // delete the connection
    delete connection_p;

    LOG_GENERATE_TRACE(g_logger, "SPPConnection::disconnect exit");
}
