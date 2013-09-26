///////////////////////////////////////////////////////////////////////////////
// includes
///////////////////////////////////////////////////////////////////////////////

#include "config.h"

extern "C" 
{
#include <ini_config.h>
};

#include <log4cxx/logger.h>

///////////////////////////////////////////////////////////////////////////////
// macros
///////////////////////////////////////////////////////////////////////////////

#define INI_APPLICATION_NAME "levelingglass"

///////////////////////////////////////////////////////////////////////////////
// type defintions
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// constants
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// module variables
///////////////////////////////////////////////////////////////////////////////

static log4cxx::LoggerPtr g_logger(log4cxx::Logger::getLogger("config"));

Config *Config::g_instance_p = NULL;

///////////////////////////////////////////////////////////////////////////////
// private function declarations
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// public function implementations
///////////////////////////////////////////////////////////////////////////////

ResultCode Config::init(const char *configpath_p)
{
    // assume success
    ResultCode result_code = RESULT_CODE_OK;

    // read the config file
    struct collection_item *ini_collection_p = NULL;
    struct collection_item *error_collection_p = NULL;
    int result = config_from_file(INI_APPLICATION_NAME, configpath_p, &ini_collection_p, INI_STOP_ON_ANY, &error_collection_p);
    switch(result)
    {
        case 0:
            // success
            LOG4CXX_DEBUG(g_logger, "successfully read config file=" + to_string(configpath_p));
            // create the singleton
            g_instance_p = new Config(ini_collection_p);
            break;

        default:
            // error
            LOG4CXX_ERROR(g_logger, "error reading config file=" + to_string(configpath_p));
            result_code = RESULT_CODE_ERROR;
            print_file_parsing_errors(stdout, error_collection_p);
            break;
    }
    if (NULL != error_collection_p)
    {
        free_ini_config_errors(error_collection_p);
    }

    return result_code;
}

///////////////////////////////////////////////////////////////////////////////
// private function implementations 
///////////////////////////////////////////////////////////////////////////////

Config::Config(struct collection_item *collection_p) : 
    m_collection_p(collection_p)
{
    LOG4CXX_TRACE(g_logger, "Config::Config enter " + to_string(collection_p));

    LOG4CXX_TRACE(g_logger, "Config::Config exit");
}

Config::~Config()
{
    LOG4CXX_TRACE(g_logger, "Config::~Config enter");
    if (NULL != m_collection_p)
    {
        free_ini_config(m_collection_p);
    }
    LOG4CXX_TRACE(g_logger, "Config::~Config exit");
}
