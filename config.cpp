///////////////////////////////////////////////////////////////////////////////
// includes
///////////////////////////////////////////////////////////////////////////////

#include "config.h"
#include "log.h"

extern "C" 
{
#include <ini_config.h>
};

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

static LogInstance g_logger("config");

Config *Config::g_instance_p = NULL;

///////////////////////////////////////////////////////////////////////////////
// private function declarations
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// public function implementations
///////////////////////////////////////////////////////////////////////////////

ResultCode Config::init(const char *configpath_p)
{
    LOG_GENERATE_TRACE(g_logger, "Config::init enter configpath_p=%s", configpath_p);

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
            LOG_GENERATE_DEBUG(g_logger, "successfully read config file=%s", configpath_p);
            // create the singleton
            g_instance_p = new Config(ini_collection_p);
            break;

        default:
            // error
            LOG_GENERATE_ERROR(g_logger, "error reading config file=%s", configpath_p);
            result_code = RESULT_CODE_ERROR;
            print_file_parsing_errors(stdout, error_collection_p);
            break;
    }
    if (NULL != error_collection_p)
    {
        free_ini_config_errors(error_collection_p);
    }

    LOG_GENERATE_TRACE(g_logger, "Config::init exit result_code=%d", result_code);
    return result_code;
}

const Config *Config::get_instance_p() 
{
    LOG_GENERATE_TRACE(g_logger, "Config::get_instance_p() enter");
    LOG_GENERATE_TRACE(g_logger, "Config::get_instance_p() exit instance_p=%p", g_instance_p);
    return g_instance_p;
}

ResultCode Config::get_float(const char *section_p, const char *item_p, float *value_p) const
{
    LOG_GENERATE_TRACE(g_logger, "Config::get_float enter this=%p section_p=%s item_p=%s value_p=%p", this, section_p, item_p, value_p);

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
        LOG_GENERATE_ERROR(g_logger, "unable to find config file time section=%s item=%s", section_p, item_p);
        result_code = RESULT_CODE_ERROR;
    }

    LOG_GENERATE_TRACE(g_logger, "Config::get_float exit result_code=%d", result_code);
    return result_code;
}

ResultCode Config::get_float_with_default(const char *section_p, const char *item_p, float default_value, float *value_p) const
{
    LOG_GENERATE_TRACE(g_logger, "Config::get_float_with_default enter this=%p section_p=%s item_p=%s default_value=%f value_p=%p", this, section_p, item_p, default_value, value_p);

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

    LOG_GENERATE_TRACE(g_logger, "Config::get_float_with_default exit result_code=%d", result_code);
    return result_code;
}

ResultCode Config::get_string(const char *section_p, const char *item_p, const char **value_pp) const
{
    LOG_GENERATE_TRACE(g_logger, "Config::get_string enter this=%p section_p=%s item_p=%s value_pp=%p", this, section_p, item_p, value_pp);

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
        LOG_GENERATE_ERROR(g_logger, "unable to find config file time section=%s item=%s", section_p, item_p);
        result_code = RESULT_CODE_ERROR;
    }

    LOG_GENERATE_TRACE(g_logger, "Config::get_string exit result_code=%d", result_code);
    return result_code;
}

ResultCode Config::get_string_with_default(const char *section_p, const char *item_p, const char *default_value_p, const char **value_pp) const
{
    LOG_GENERATE_TRACE(g_logger, "Config::get_string_with_default enter this=%p section_p=%s item_p=%s default_value_p=%s value_pp=%p", this, section_p, item_p, default_value_p, value_pp);

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

    LOG_GENERATE_TRACE(g_logger, "Config::get_string_with_default exit result_code=%d", result_code);
    return result_code;
}

///////////////////////////////////////////////////////////////////////////////
// private function implementations 
///////////////////////////////////////////////////////////////////////////////

Config::Config(struct collection_item *collection_p) : 
    m_collection_p(collection_p)
{
    LOG_GENERATE_TRACE(g_logger, "Config::Config enter this=%p collection_p=%p", this, collection_p);
    LOG_GENERATE_TRACE(g_logger, "Config::Config exit");
}

Config::~Config()
{
    LOG_GENERATE_TRACE(g_logger, "Config::~Config enter this=%p", this);
    if (NULL != m_collection_p)
    {
        free_ini_config(m_collection_p);
    }
    LOG_GENERATE_TRACE(g_logger, "Config::~Config exit");
}
