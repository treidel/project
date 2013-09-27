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
    LOG4CXX_TRACE(g_logger, "Config::init enter " + to_string(configpath_p));

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

    LOG4CXX_TRACE(g_logger, "Config::init exit " + to_string(result_code));
    return result_code;
}

const Config *Config::get_instance_p() 
{
    LOG4CXX_TRACE(g_logger, "Config::get_instance_p() enter");
    LOG4CXX_TRACE(g_logger, "Config::get_instance_p() exit instance_p=" + to_string(g_instance_p));
    return g_instance_p;
}

ResultCode Config::get_float(const char *section_p, const char *item_p, float *value_p) const
{
    LOG4CXX_TRACE(g_logger, "Config::get_float enter " + to_string(section_p) + " " + to_string(item_p) + " " + to_string(value_p));

    ResultCode result_code = RESULT_CODE_OK;

    // query for the item
    struct collection_item *item_collection_p = NULL;
    if(0 == get_config_item(section_p, item_p, m_collection_p, &item_collection_p))
    {
        // get the value from the collection
        double value = get_double_config_value(item_collection_p, 1, 0.0, NULL);
        *value_p = (float)value;
    }
    else
    {
        LOG4CXX_ERROR(g_logger, "unable to find config file time section=" + to_string(section_p) + " item=" + to_string(item_p));
        result_code = RESULT_CODE_ERROR;
    }

    LOG4CXX_TRACE(g_logger, "Config::get_float exit " + to_string(result_code));
    return result_code;
}

ResultCode Config::get_float_with_default(const char *section_p, const char *item_p, float default_value, float *value_p) const
{
    LOG4CXX_TRACE(g_logger, "Config::get_float_with_default enter " + to_string(section_p) + " " + to_string(item_p) + " " + to_string(default_value) + " " + to_string(value_p));

    ResultCode result_code = RESULT_CODE_OK;

    // query for the item
    struct collection_item *item_collection_p = NULL;
    if(0 == get_config_item(section_p, item_p, m_collection_p, &item_collection_p))
    {
        // get the value from the collection
        double value = get_double_config_value(item_collection_p, 1, 0.0, NULL);
        *value_p = (float)value;
    }
    else
    {
        // set the default value
        *value_p = default_value;
    }

    LOG4CXX_TRACE(g_logger, "Config::get_float_with_default exit " + to_string(result_code));
    return result_code;
}

ResultCode Config::get_string(const char *section_p, const char *item_p, const char **value_pp) const
{
    LOG4CXX_TRACE(g_logger, "Config::get_string enter " + to_string(section_p) + " " + to_string(item_p) + " " + to_string(value_pp));

    ResultCode result_code = RESULT_CODE_OK;

    // query for the item
    struct collection_item *item_collection_p = NULL;
    if(0 == get_config_item(section_p, item_p, m_collection_p, &item_collection_p))
    {
        // get the value from the collection
        *value_pp = get_const_string_config_value(item_collection_p, NULL);
    }
    else
    {
        LOG4CXX_ERROR(g_logger, "unable to find config file time section=" + to_string(section_p) + " item=" + to_string(item_p));
        result_code = RESULT_CODE_ERROR;
    }

    LOG4CXX_TRACE(g_logger, "Config::get_string exit " + to_string(result_code));
    return result_code;
}

ResultCode Config::get_string_with_default(const char *section_p, const char *item_p, const char *default_value_p, const char **value_pp) const
{
    LOG4CXX_TRACE(g_logger, "Config::get_string_with_default enter " + to_string(section_p) + " " + to_string(item_p) + " " + to_string(default_value_p) + " " + to_string(value_pp));

    ResultCode result_code = RESULT_CODE_OK;

    // query for the item
    struct collection_item *item_collection_p = NULL;
    if(0 == get_config_item(section_p, item_p, m_collection_p, &item_collection_p))
    {
        // get the value from the collection
        *value_pp = get_const_string_config_value(item_collection_p, NULL);
    }
    else
    {
        *value_pp = default_value_p;
    }

    LOG4CXX_TRACE(g_logger, "Config::get_string_with_default exit " + to_string(result_code));
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
