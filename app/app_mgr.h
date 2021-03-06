#ifndef _APP_MGR_H_
#define _APP_MGR_H_

#include "common.h"

///////////////////////////////////////////////////////////////////////////////
// macros
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// forward declarations
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// type definitions
///////////////////////////////////////////////////////////////////////////////


class APPManager
{

public:

    class Message 
    {
    public:
        Message(const size_t length);
        ~Message();

        inline const size_t get_length() const;
        inline uint8_t *get_data_p();

    private:

        size_t m_length;
        uint8_t *m_data_p;
    };

    class RequestHandler
    {
    public:
        virtual ~RequestHandler();
        virtual ResultCode handle_request(Message *request_p, Message **response_pp) = 0;
    };

    class NotificationHandler
    {
    public:
        virtual ResultCode send_notification(Message **notification_pp) = 0;
    };

///////////////////////////////////////////////////////////////////////////////
// public function declarations
///////////////////////////////////////////////////////////////////////////////

public:

    static RequestHandler *createSPPConnector(NotificationHandler *handler_p);


///////////////////////////////////////////////////////////////////////////////
// private variable definitions
///////////////////////////////////////////////////////////////////////////////

private:

};

const size_t APPManager::Message::get_length() const
{
    return m_length;
}
 
uint8_t *APPManager::Message::get_data_p()
{
    return m_data_p;
}

#endif
