#ifndef _SPP_CONNECTION_H_
#define _SPP_CONNECTION_H_

#include "common.h"
#include "app/app_mgr.h"

#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>

#include <ev.h>

#include <string>

///////////////////////////////////////////////////////////////////////////////
// macros
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// forward declarations
///////////////////////////////////////////////////////////////////////////////

class SPPServer;

///////////////////////////////////////////////////////////////////////////////
// class declaration
///////////////////////////////////////////////////////////////////////////////

class SPPConnection : public APPManager::NotificationHandler
{

///////////////////////////////////////////////////////////////////////////////
// type definitions
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// public function declarations
///////////////////////////////////////////////////////////////////////////////

public:
    SPPConnection(SPPServer *server_p, int socket, const bdaddr_t *remote_addr_p);
    virtual ~SPPConnection();

    static std::string format_mac_addr(const bdaddr_t *addr_p);

///////////////////////////////////////////////////////////////////////////////
// APPManager:NotificationHandler declarations
///////////////////////////////////////////////////////////////////////////////

public:
    ResultCode send_notification(APPManager::Message **notification_pp);


///////////////////////////////////////////////////////////////////////////////
// private function declarations
///////////////////////////////////////////////////////////////////////////////

private:
    static void socket_cb(EV_P_ ev_io *w_p, int revents);
    static void send_cb(EV_P_ ev_io *w_p, int revents);

///////////////////////////////////////////////////////////////////////////////
// private variable declarations
///////////////////////////////////////////////////////////////////////////////

private:
    SPPServer *m_server_p;
    APPManager::RequestHandler *m_handler_p;
    struct ev_loop *m_loop_p;
    bdaddr_t m_remote_addr;
    ev_io m_watcher;
    int m_socket;
};

#endif
