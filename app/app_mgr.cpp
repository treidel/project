
#include "app_mgr.h"
#include "app_sppconnector.h"
#include "log.h"

#include <stdlib.h>

///////////////////////////////////////////////////////////////////////////////
// macros
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// type defintions
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// constants
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// module variables
///////////////////////////////////////////////////////////////////////////////

static LogInstance g_logger("app.mgr");

///////////////////////////////////////////////////////////////////////////////
// private function declarations
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// public function implementations
///////////////////////////////////////////////////////////////////////////////

APPManager::RequestHandler *APPManager::createSPPConnector(NotificationHandler *handler_p)
{
    LOG_GENERATE_TRACE(g_logger, "APPManager::createSPPConnector enter handler_p=%p", handler_p);

    SPPConnector *connector_p = new SPPConnector(handler_p);

    LOG_GENERATE_TRACE(g_logger, "APPManager::createSPPConnector exit connector_p=%p", connector_p);
    return connector_p;
}

APPManager::RequestHandler::~RequestHandler()
{
    LOG_GENERATE_TRACE(g_logger, "APPManager::RequestHandler::~RequestHandler enter this=%p", this);
    LOG_GENERATE_TRACE(g_logger, "APPManager::RequestHandler::~RequestHandler exit");
}

APPManager::Message::Message(const size_t length) :
    m_length(length),
    m_data_p((uint8_t *)malloc(length))
{
}

APPManager::Message::~Message()
{
    if (NULL != m_data_p)
    {
        free(m_data_p);
    }
}

