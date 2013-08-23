
#include "bluetooth/spp_server.h"
#include "audio/audio_capturemgr.h"

//#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <ev.h>

#include <log4cxx/logger.h>
#include <log4cxx/propertyconfigurator.h>

///////////////////////////////////////////////////////////////////////////////
// macros
///////////////////////////////////////////////////////////////////////////////

#define NAME "Leveling Glass"
#define DEFAULT_LOG_CONFIG_FILE "/etc/leveling-glass/log.cfg"

// make sure the version macro is defined
#ifndef VERSION
#error VERSION is not defined
#endif

///////////////////////////////////////////////////////////////////////////////
// type defintions
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// constants
///////////////////////////////////////////////////////////////////////////////

// UUID=c20d3a1a-6c1d-11e2-aa09-000c298ce626
static const uint8_t c_uuid_int[] = {0xc2, 0x0d, 0x3a, 0x1a, 0x6c, 0xd, 0x11, 0xe2, 0xaa, 0x09, 0x00, 0x0c, 0x29, 0x8c, 0xe6, 0x26};

///////////////////////////////////////////////////////////////////////////////
// module variables
///////////////////////////////////////////////////////////////////////////////

static uuid_t g_uuid;
static SPPServer *g_server_p = NULL;
static log4cxx::LoggerPtr g_logger(log4cxx::Logger::getLogger("main"));
static const char* g_default_log_config_p = DEFAULT_LOG_CONFIG_FILE;

static struct option g_options[] = 
{
    {"config", optional_argument, 0, 'c'},
    {0, 0, 0, 0}
};

///////////////////////////////////////////////////////////////////////////////
// private function declarations
///////////////////////////////////////////////////////////////////////////////

static void kill_cb(struct ev_loop *loop_p, ev_signal *w_p, int revents);
static void populate_logpath(char *execpath_p, size_t path_length);

///////////////////////////////////////////////////////////////////////////////
// public function implementations
///////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
    // setup default parameters
    const char *config_p = g_default_log_config_p;

    // parse the command line
    while(true)
    {
        int option_index = 0;
        int c = getopt_long(argc, argv, "c:", g_options, &option_index);
        // detect end of options
        if (-1 == c)
        {
            break;
        }

        switch (c)
        {
            case 0:
                switch(g_options[option_index].val)
                {
                    case 'c':
                        LOG4CXX_INFO(g_logger, "using log config file=" + to_string(optarg));
                        config_p = optarg;
                        break;
                    default:
                        LOG4CXX_FATAL(g_logger, "invalid long option " + to_string(g_options[option_index].name));
                        return -1;
                }
                break;
            case 'c':
                LOG4CXX_INFO(g_logger, "using log config file=" + to_string(optarg));
                config_p = optarg; 
                break;
            default:
                LOG4CXX_FATAL(g_logger, "invalid option '" + to_string(c) + "'");
                return -1;
        }
    }

    // setup the logging layer
    log4cxx::PropertyConfigurator::configure(config_p);

    LOG4CXX_INFO(g_logger, "Starting " << NAME << " - version " << VERSION);

    // create the UUID for our SPP server
    sdp_uuid128_create(&g_uuid, &c_uuid_int);
    // start the Bluetooth SPP server
    g_server_p = new SPPServer(g_uuid);

    // create the audio layer
    AUDIOCaptureManager::get_instance();

    // setup the default event loop
    struct ev_loop *loop_p = EV_DEFAULT;
    // register a signal handler for the KILL event
    ev_signal kill_watcher;
    ev_signal_init(&kill_watcher, kill_cb, SIGKILL);
    ev_signal_start(loop_p, &kill_watcher);

    // notify that we're running
    LOG4CXX_INFO(g_logger, NAME << " running");

    // run forever
    ev_run(loop_p, 0);

    // we should never get here
    LOG4CXX_FATAL(g_logger, "main loop exited");
    return -1;
}

const std::string to_string(int value)
{
    std::stringstream ss;
    std::string s;
    ss << value;
    s = ss.str();
    return s;
}

const std::string to_string(uint8_t value)
{
    std::stringstream ss;
    std::string s;
    ss << (int)value;
    s = ss.str();
    return s;
}

const std::string to_string(uint8_t *data_p, size_t length)
{
  char buffer[(2 * length) + 1];
  for (int counter = 0; counter < length; counter++)
  {
      uint8_t value = *(data_p + counter);
      snprintf(buffer + (2 * counter), 3, "%02X", value);
  }
  return buffer;
}

const std::string to_string(const char *string_p)
{
    std::string value(string_p);
    return value;
}

///////////////////////////////////////////////////////////////////////////////
// private function implementations 
///////////////////////////////////////////////////////////////////////////////

void kill_cb(struct ev_loop *loop_p, ev_signal *w_p, int revents)
{
    LOG4CXX_INFO(g_logger, "Exiting");
    exit(1);
}

