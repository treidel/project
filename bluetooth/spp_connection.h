#ifndef _SPP_CONNECTION_H_
#define _SPP_CONNECTION_H_

#include "common.h"
#include "app/app_mgr.h"

#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>

#include <ev.h>

#include <string>
#include <queue>

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

private:

	class Buffer
	{
    public:
        Buffer();
        ~Buffer();

        void clear();
        size_t calculate_offset();

        inline bool is_empty();

        void set_message(APPManager::Message *message_p);
        inline APPManager::Message *get_message_p();

        inline size_t &get_remaining_r();

    private:
	    APPManager::Message *m_message_p;
        size_t m_remaining;
	};

///////////////////////////////////////////////////////////////////////////////
// public function declarations
///////////////////////////////////////////////////////////////////////////////

public:
    SPPConnection(SPPServer *server_p, int socket, const bdaddr_t *remote_addr_p);
    virtual ~SPPConnection();

    static const std::string format_mac_addr(const bdaddr_t *addr_p);

///////////////////////////////////////////////////////////////////////////////
// APPManager:NotificationHandler declarations
///////////////////////////////////////////////////////////////////////////////

public:

    ResultCode send_notification(APPManager::Message **notification_pp);


///////////////////////////////////////////////////////////////////////////////
// private function declarations
///////////////////////////////////////////////////////////////////////////////

private:
    static void receive_cb(EV_P_ ev_io *w_p, int revents);
    static void transmit_cb(EV_P_ ev_io *w_p, int revents);
    static void disconnect(SPPConnection **connection_pp);

///////////////////////////////////////////////////////////////////////////////
// private variable declarations
///////////////////////////////////////////////////////////////////////////////

private:
    SPPServer *m_server_p;
    APPManager::RequestHandler *m_handler_p;
    struct ev_loop *m_loop_p;
    bdaddr_t m_remote_addr;
    ev_io m_receive_watcher;
    ev_io m_transmit_watcher;
    std::queue<APPManager::Message *> m_transmit_queue;
    Buffer m_receive;
    Buffer m_transmit;
    int m_socket;
};

bool SPPConnection::Buffer::is_empty()
{
    return (NULL != m_message_p);
}

APPManager::Message *SPPConnection::Buffer::get_message_p()
{
    return m_message_p;
}

size_t &SPPConnection::Buffer::get_remaining_r()
{
    return m_remaining;
}

#endif
