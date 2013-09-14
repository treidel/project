///////////////////////////////////////////////////////////////////////////////
// includes
///////////////////////////////////////////////////////////////////////////////

#include "app_sppconnector.h"
#include "control/control.h"
#include "audio/audio_processor.h"

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

static log4cxx::LoggerPtr g_logger(log4cxx::Logger::getLogger("app.sppconnector"));

///////////////////////////////////////////////////////////////////////////////
// private function declarations
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// public function implementations
///////////////////////////////////////////////////////////////////////////////

SPPConnector::SPPConnector(APPManager::NotificationHandler *handler_p) :
    m_processor_p(new AUDIOProcessor()),
    m_control_p(new Control(m_processor_p, handler_p))
{
    LOG4CXX_TRACE(g_logger, "SPPConnector::SPPConnector enter " << handler_p);
    LOG4CXX_TRACE(g_logger, "SPPConnector::SPPConnector exit");
}

SPPConnector::~SPPConnector()
{
    LOG4CXX_TRACE(g_logger, "SPPConnector::~SPPConnector enter");
    delete m_control_p;
    delete m_processor_p;
    LOG4CXX_TRACE(g_logger, "SPPConnector::~SPPConnector exit");
}

///////////////////////////////////////////////////////////////////////////////
// APPManager::RequestHandler implementations
///////////////////////////////////////////////////////////////////////////////

ResultCode SPPConnector::handle_request(APPManager::Message *request_p, APPManager::Message **response_pp)
{
    LOG4CXX_TRACE(g_logger, "SPPConnector::handle_request enter");

    // pass thru the request to the control object
    ResultCode result = m_control_p->handle_request(request_p, response_pp);

    LOG4CXX_TRACE(g_logger, "SPPConnector::handle_request exit " << result);
    return result;
}



