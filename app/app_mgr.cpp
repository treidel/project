
#include "app_mgr.h"
#include "app_sppconnector.h"

#include <log4cxx/logger.h>

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
static log4cxx::LoggerPtr g_logger(log4cxx::Logger::getLogger("app.mgr"));

///////////////////////////////////////////////////////////////////////////////
// private function declarations
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// public function implementations
///////////////////////////////////////////////////////////////////////////////

APPManager::RequestHandler *APPManager::createSPPConnector(NotificationHandler *handler_p)
{
    LOG4CXX_DEBUG(g_logger, "APPManager::createSPPConnector enter " << handler_p);

    SPPConnector *connector_p = new SPPConnector(handler_p);

    LOG4CXX_DEBUG(g_logger, "APPManager::createSPPConnector exit " << connector_p);

    return connector_p;
}

