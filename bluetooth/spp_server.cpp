#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include <sys/socket.h>

#include "common.h"
#include "spp_server.h"
#include "spp_connection.h"

///////////////////////////////////////////////////////////////////////////////
// type defintions
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// constants
///////////////////////////////////////////////////////////////////////////////

static const bdaddr_t g_bluetooth_any_addr  = {0, 0, 0, 0, 0, 0};
static const bdaddr_t g_bluetooth_all_addr = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
static const bdaddr_t g_bluetooth_local_addr = {0, 0, 0, 0xff, 0xff, 0xff};

static const char *g_servicename = "LevelingGlass Bluetooth SPP V1.0 Service";
static const char *g_servicedescription = "Version 1.0 of the Bluetooth SPP Service for LevelingGlass";
static const char *g_serviceprovider = "timmytimmytimmy.com";

///////////////////////////////////////////////////////////////////////////////
// module variables
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// private function declarations
///////////////////////////////////////////////////////////////////////////////

static uint8_t allocate_channel(int sock, struct sockaddr_rc *sockaddr);


///////////////////////////////////////////////////////////////////////////////
// public function implementations
///////////////////////////////////////////////////////////////////////////////

SPPServer::SPPServer(uuid_t uuid) :
	m_uuid(uuid),
	m_session_p(NULL),
	m_loop_p(ev_default_loop(0)),
	m_socket(0),
	m_connection_p(NULL)
{

	// create the SPP socket
        m_socket = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
        if (0 > m_socket)
        {
            // TBD: log error
            return;
        }

        // bind socket to the first RFCOMM port on the first available 
        // local bluetooth adapter
        struct sockaddr_rc loc_addr = { 0, 0, 0 };
        bdaddr_t addr = { 0 };
        loc_addr.rc_family = AF_BLUETOOTH;
        loc_addr.rc_bdaddr = addr;
        loc_addr.rc_channel = (uint8_t)0;
        uint8_t channel = allocate_channel(m_socket, &loc_addr);

        // listen for connections with no backlog 
        listen(m_socket, 0);
    
        // create the SDP record
        sdp_record_t *record_p = sdp_record_alloc();

        // set the general service ID
        sdp_set_service_id(record_p, m_uuid);

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
        sdp_set_info_attr(record_p, g_servicename, g_serviceprovider, g_servicedescription);

	// connect to SDP
	m_session_p = sdp_connect(&g_bluetooth_any_addr, &g_bluetooth_local_addr, SDP_RETRY_IF_BUSY);
	// register the service
	if (0 > sdp_record_register(m_session_p, record_p, 0))
	{
		// TBD: error
		return;
	}
	
	// cleanup
	sdp_data_free(channel_p);
	sdp_list_free(l2cap_list_p, 0);
	sdp_list_free(rfcomm_list_p, 0);
	sdp_list_free(root_list_p, 0);
	sdp_list_free(access_proto_list_p, 0);

   	// initialize an io watcher for the socket
    	ev_io_init(&m_watcher, socket_cb, m_socket, EV_READ);
	m_watcher.data = (void *)this;
    
    	// register the listener with the loop
    	ev_io_start(m_loop_p, &m_watcher);
}

SPPServer::~SPPServer()
{
	// stop the watcher
	ev_io_stop(m_loop_p, &m_watcher);
	// close the SDP session
	sdp_close(m_session_p);
	// close the socket
	close(m_socket);
}

///////////////////////////////////////////////////////////////////////////////
// private function implementations
///////////////////////////////////////////////////////////////////////////////

uint8_t allocate_channel(int sock, struct sockaddr_rc *sockaddr_p)
{
    	for (int port= 1; port <= 31; port++) 
    	{
        	sockaddr_p->rc_channel = port;
        	int err = bind(sock, (struct sockaddr *)sockaddr_p, sizeof(struct sockaddr_rc));
        	if ((0 != err) || (errno == EINVAL))
		{ 
           		return port; 
        	}
    	}
	// if we get here it was an error
    	return 0;
}

void SPPServer::socket_cb (EV_P_ ev_io *w, int revents)
{
	// get the object
	SPPServer *server_p = (SPPServer *)(w->data);
    	// we're only signalled when a new connection has arrived so let's accept it
    	struct sockaddr_rc remote_addr;
    	socklen_t len = sizeof(struct sockaddr_rc);
    	int client_socket = accept(w->fd, (struct sockaddr *)&remote_addr, &len);
    	if (0 > client_socket)
    	{
        	// TBD: error log
        	return;
    	}

    	// TBD: info log
   
    	// see if we already have a connection 
    	if (NULL == server_p->m_connection_p)
    	{
        	// create the connection
		server_p->m_connection_p = new SPPConnection(server_p, client_socket);
    	}
    	else
    	{
        	// TBD: warning log

        	// close the socket unceremoniously 
        	close(client_socket);
    	}

    	// done
}

