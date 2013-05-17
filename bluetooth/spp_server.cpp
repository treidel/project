#include "spp_server.h"
#include "spp_connection.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>

#include <string>

#include <log4cxx/logger.h>

///////////////////////////////////////////////////////////////////////////////
// macros
///////////////////////////////////////////////////////////////////////////////

#define MAC_ADDR_STRING_LENGTH ((2*6)+5)

///////////////////////////////////////////////////////////////////////////////
// type defintions
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// constants
///////////////////////////////////////////////////////////////////////////////

static const bdaddr_t g_bluetooth_any_addr = { 0, 0, 0, 0, 0, 0 };
static const bdaddr_t g_bluetooth_all_addr = { 0xff, 0xff, 0xff, 0xff, 0xff,
                      0xff
                                             };
static const bdaddr_t g_bluetooth_local_addr = { 0, 0, 0, 0xff, 0xff, 0xff };

static const char *g_servicename = "LevelingGlass Bluetooth SPP V1.0 Service";
static const char *g_servicedescription =
    "Version 1.0 of the Bluetooth SPP Service for LevelingGlass";
static const char *g_serviceprovider = "Wazzup";

///////////////////////////////////////////////////////////////////////////////
// module variables
///////////////////////////////////////////////////////////////////////////////

static log4cxx::LoggerPtr g_logger(
    log4cxx::Logger::getLogger("bluetooth.spp.server"));

///////////////////////////////////////////////////////////////////////////////
// private function declarations
///////////////////////////////////////////////////////////////////////////////

static uint8_t allocate_channel(int sock, struct sockaddr_rc *sockaddr);
static std::string format_mac_addr(const bdaddr_t *addr_p);

///////////////////////////////////////////////////////////////////////////////
// public function implementations
///////////////////////////////////////////////////////////////////////////////

SPPServer::SPPServer(uuid_t uuid) :
    m_uuid(uuid), m_session_p(NULL), m_loop_p(ev_default_loop(0)), m_socket(
        0), m_connection_p(NULL)
{
    LOG4CXX_DEBUG(g_logger, "SPPServer::SPPServer enter");

    // create the SPP socket
    m_socket = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
    if (0 > m_socket)
    {
        LOG4CXX_ERROR(g_logger, "socket returned error " << m_socket);
        return;
    }

    // bind socket to the first RFCOMM port on the first available
    // local bluetooth adapter
    struct sockaddr_rc loc_addr;
    memset(&loc_addr, 0, sizeof(loc_addr));
    loc_addr.rc_family = AF_BLUETOOTH;
    memcpy(&loc_addr.rc_bdaddr, &g_bluetooth_any_addr, sizeof(bdaddr_t));
    loc_addr.rc_channel = 0;

    // allocate the channel
    uint8_t channel = allocate_channel(m_socket, &loc_addr);

    LOG4CXX_DEBUG(g_logger, "SPPServer::SPPServer channel=" << loc_addr.rc_channel);

    // listen for connections with no backlog
    listen(m_socket, 0);

    // create the SDP record
    sdp_record_t *record_p = sdp_record_alloc();

    // setup the service class id list
    sdp_list_t *service_class_id_list_p = sdp_list_append(0, &m_uuid);
    sdp_set_service_classes(record_p, service_class_id_list_p);

    // make the service record publicly browsable
    uuid_t root_uuid;
    sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
    sdp_list_t *root_list_p = sdp_list_append(0, &root_uuid);
    sdp_set_browse_groups(record_p, root_list_p);

    // set l2cap information
    uuid_t l2cap_uuid;
    sdp_uuid16_create(&l2cap_uuid, L2CAP_UUID);
    sdp_list_t *l2cap_list_p = sdp_list_append(0, &l2cap_uuid);
    sdp_list_t *proto_list_p = sdp_list_append(0, l2cap_list_p);

    // set rfcomm information
    uuid_t rfcomm_uuid;
    sdp_uuid16_create(&rfcomm_uuid, RFCOMM_UUID);
    sdp_data_t *channel_p = sdp_data_alloc(SDP_UINT8, &channel);
    sdp_list_t *rfcomm_list_p = sdp_list_append(0, &rfcomm_uuid);
    sdp_list_append(rfcomm_list_p, channel_p);
    sdp_list_append(proto_list_p, rfcomm_list_p);

    // attach protocol information to service record
    sdp_list_t *access_proto_list_p = sdp_list_append(0, proto_list_p);
    sdp_set_access_protos(record_p, access_proto_list_p);

    // set the name, provider, and description
    sdp_set_info_attr(record_p, g_servicename, g_serviceprovider,
                      g_servicedescription);

    // connect to SDP
    m_session_p = sdp_connect(&g_bluetooth_any_addr, &g_bluetooth_local_addr,
                              SDP_RETRY_IF_BUSY);
    // register the service
    int rc = sdp_record_register(m_session_p, record_p, 0);
    if (0 > rc)
    {
        LOG4CXX_ERROR(g_logger, "sdp_record_register returned error " << rc);
        return;
    }

    // cleanup
    sdp_data_free(channel_p);
    sdp_list_free(l2cap_list_p, 0);
    sdp_list_free(rfcomm_list_p, 0);
    sdp_list_free(root_list_p, 0);
    sdp_list_free(access_proto_list_p, 0);
    sdp_list_free(service_class_id_list_p, 0);

    // initialize an io watcher for the socket
    ev_io_init(&m_watcher, socket_cb, m_socket, EV_READ);
    m_watcher.data = (void *) this;

    // register the listener with the loop
    ev_io_start(m_loop_p, &m_watcher);

    LOG4CXX_DEBUG(g_logger, "SPPServer::SPPServer exit");
}

SPPServer::~SPPServer()
{
    LOG4CXX_DEBUG(g_logger, "SPPServer::~SPPServer enter");

    // stop the watcher
    ev_io_stop(m_loop_p, &m_watcher);
    // close the SDP session
    sdp_close(m_session_p);
    // close the socket
    close(m_socket);

    LOG4CXX_DEBUG(g_logger, "SPPServer::~SPPServer exit");
}

///////////////////////////////////////////////////////////////////////////////
// private function implementations
///////////////////////////////////////////////////////////////////////////////

void SPPServer::socket_cb(EV_P_ ev_io *w_p, int revents)
{
    LOG4CXX_DEBUG(g_logger, "SPPServer::socket_cb enter " << w_p << " " <<revents);
    // get the object
    SPPServer *server_p = (SPPServer *)(w_p->data);
    // we're only signalled when a new connection has arrived so let's accept it
    struct sockaddr_rc remote_addr;
    socklen_t len = sizeof(struct sockaddr_rc);
    int client_socket = accept(w_p->fd, (struct sockaddr *)&remote_addr, &len);
    if (0 > client_socket)
    {
        LOG4CXX_ERROR(g_logger, "accept returned error " << client_socket);
        return;
    }

    std::string mac_addr_s = format_mac_addr(&remote_addr.rc_bdaddr);
    LOG4CXX_INFO(g_logger, "received connection from " << mac_addr_s);

    // see if we already have a connection
    if (NULL == server_p->m_connection_p)
    {
        // create the connection
        server_p->m_connection_p = new SPPConnection(server_p, client_socket);
    }
    else
    {
        LOG4CXX_WARN(g_logger, "refusing connection request -> already connected");

        // close the socket unceremoniously
        close(client_socket);
    }

    // done
    LOG4CXX_DEBUG(g_logger, "SPPServer::socket_cb exit");
}

uint8_t allocate_channel(int sock, struct sockaddr_rc *sockaddr_p)
{
    for (int port = 2; port <= 31; port++)
    {
        sockaddr_p->rc_channel = port;
        int err = bind(sock, (struct sockaddr *) sockaddr_p,
                       sizeof(struct sockaddr_rc));
        if ((0 == err) && (errno != EINVAL))
        {
            return port;
        }
    }
    // if we get here it was an error
    return 0;
}

std::string format_mac_addr(const bdaddr_t *addr_p)
{
    std::string buffer_s(MAC_ADDR_STRING_LENGTH, ' ');
    snprintf((char *) buffer_s.c_str(), buffer_s.capacity(),
             "%02X:%02X:%02X:%02X:%02X:%02X", addr_p->b[0], addr_p->b[1],
             addr_p->b[2], addr_p->b[3], addr_p->b[4], addr_p->b[5]);
    return buffer_s;
}

