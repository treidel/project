///////////////////////////////////////////////////////////////////////////////
// includes
///////////////////////////////////////////////////////////////////////////////

#include "log.h"

#include <stdio.h>

///////////////////////////////////////////////////////////////////////////////
// macros
///////////////////////////////////////////////////////////////////////////////

#define LOG_MESSAGE_LENGTH_MAX (255)

///////////////////////////////////////////////////////////////////////////////
// type defintions
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// constants
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// module variables
///////////////////////////////////////////////////////////////////////////////

LogInstance::LogLevel LogInstance::g_level = LOG_LEVEL_INFO;

///////////////////////////////////////////////////////////////////////////////
// private function declarations
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// public function implementations
///////////////////////////////////////////////////////////////////////////////

LogInstance::LogInstance(const char *name_p)
    : m_logptr(log4cxx::Logger::getLogger(name_p))
{
}

LogInstance::~LogInstance()
{
}

void LogInstance::set_global_level(LogLevel level)
{
    g_level = level;
}

///////////////////////////////////////////////////////////////////////////////
// private function implementations
///////////////////////////////////////////////////////////////////////////////

void LogInstance::generate_internal(LogLevel level, const char *format_p, va_list va)
{
    // generate the log text
    char message[LOG_MESSAGE_LENGTH_MAX + 1];
    vsnprintf(message, sizeof(message), format_p, va);
    
    // output to the log4xx layer
    switch (level)
    {
        case LOG_LEVEL_INFO:
            LOG4CXX_INFO(m_logptr, message);
            break;
        case LOG_LEVEL_WARNING:
            LOG4CXX_WARN(m_logptr, message);
            break;
        case LOG_LEVEL_DEBUG:
            LOG4CXX_DEBUG(m_logptr, message);
            break;
        case LOG_LEVEL_TRACE:
            LOG4CXX_DEBUG(m_logptr, message);
            break;
        case LOG_LEVEL_ERROR:
            LOG4CXX_ERROR(m_logptr, message);
            break;
        case LOG_LEVEL_WTF:
            LOG4CXX_FATAL(m_logptr, message);
            break;
        default:
            // should never happen
            return;
    }
}

///////////////////////////////////////////////////////////////////////////////
// private function implementations 
///////////////////////////////////////////////////////////////////////////////


