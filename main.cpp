
#include "bluetooth/spp_server.h"
#include "audio/audio_capturemgr.h"
#include "config.h"

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
#define DEFAULT_APP_CONFIG_FILE "/etc/leveling-glass/leveling-glass.ini"

// make sure the version macro is defined
#ifndef VERSION
    #define VERSION "INTERNAL"
#endif

///////////////////////////////////////////////////////////////////////////////
// type defintions
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// constants
///////////////////////////////////////////////////////////////////////////////

// UUID=c20d3a1a-6c1d-11e2-aa09-000c298ce626
static const uint8_t c_uuid_int[] = {0xc2, 0x0d, 0x3a, 0x1a, 0x6c, 0xd, 0x11, 0xe2, 0xaa, 0x09, 0x00, 0x0c, 0x29, 0x8c, 0xe6, 0x26};
static const char *c_version = VERSION;

///////////////////////////////////////////////////////////////////////////////
// module variables
///////////////////////////////////////////////////////////////////////////////

static uuid_t g_uuid;
static SPPServer *g_server_p = NULL;
static log4cxx::LoggerPtr g_logger(log4cxx::Logger::getLogger("main"));
static const char *g_default_log_config_p = DEFAULT_LOG_CONFIG_FILE;
static const char *g_default_app_config_p = DEFAULT_APP_CONFIG_FILE; 

static struct option g_options[] = 
{
    {"config", optional_argument, 0, 'c'},
    {"log", optional_argument, 0, 'l'},
    {"daemon", optional_argument, 0, 'd'},
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
    bool daemon = false;
    const char *app_config_p = g_default_app_config_p;
    const char *log_config_p = g_default_log_config_p;

    // parse the command line
    while(true)
    {
        int option_index = 0;
        int c = getopt_long(argc, argv, "dcl:", g_options, &option_index);
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
                        LOG4CXX_INFO(g_logger, "using app config file=" + to_string(optarg));
                        app_config_p = optarg;
                        break;

                    case 'l':
                        LOG4CXX_INFO(g_logger, "using log config file=" + to_string(optarg));
                        log_config_p = optarg;
                        break;
                    case 'd':
                        LOG4CXX_INFO(g_logger, "running in background");
                        daemon = true;
                        break;
                    default:
                        LOG4CXX_FATAL(g_logger, "invalid long option " + to_string(g_options[option_index].name));
                        return -1;
                }
                break;
            case 'c':
                LOG4CXX_INFO(g_logger, "using app config file=" + to_string(optarg));
                app_config_p = optarg; 
                break;                
            case 'l':
                LOG4CXX_INFO(g_logger, "using log config file=" + to_string(optarg));
                log_config_p = optarg; 
                break;
            case 'd':
                LOG4CXX_INFO(g_logger, "running in background");
                daemon = true;
                break;
            default:
                LOG4CXX_FATAL(g_logger, "invalid option '" + to_string(c) + "'");
                return -1;
        }
    }

    // setup the logging layer
    log4cxx::PropertyConfigurator::configure(log_config_p);

    LOG4CXX_INFO(g_logger, "Starting " << NAME << " - version " << c_version);

    // read the config file 
    ResultCode result_code = Config::init(app_config_p);
    if (RESULT_CODE_OK != result_code)
    {
        LOG4CXX_FATAL(g_logger, "unable to read app config file due to rc=" + to_string(result_code) + ", exiting");
        return -1;
    }

    // create the UUID for our SPP server
    sdp_uuid128_create(&g_uuid, &c_uuid_int);
    // start the Bluetooth SPP server
    g_server_p = new SPPServer(g_uuid);

    // create the audio layer
    AUDIOCaptureManager::get_instance();

    // setup the default event loop
    struct ev_loop *loop_p = EV_DEFAULT;

    // register signal handlers for the TERM + KILL signals
    ev_signal kill_watcher;
    ev_signal_init(&kill_watcher, kill_cb, SIGKILL);
    ev_signal_start(loop_p, &kill_watcher);
    ev_signal term_watcher;
    ev_signal_init(&term_watcher, kill_cb, SIGTERM);
    ev_signal_start(loop_p, &term_watcher);

    // see if we're going to run in daemon mone
    if (true == daemon)
    {
        // fork into two processes
        pid_t pid = fork();
        switch (pid)
        {
            case -1:
                LOG4CXX_FATAL(g_logger, "unable to fork");
                return -1;
            case 0:
                LOG4CXX_DEBUG(g_logger, "running in background");
                // help libev handle the fork
                ev_default_fork();
                break;
            default:
                LOG4CXX_INFO(g_logger, "background daemon process pid=" + to_string(pid));
                return 0;
        }
    }

    // notify that we're running
    LOG4CXX_INFO(g_logger, NAME << " running");

    // run forever
    ev_run(loop_p, 0);

    // we should never get here
    LOG4CXX_FATAL(g_logger, "main loop exited");
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
// private function implementations 
///////////////////////////////////////////////////////////////////////////////

void kill_cb(struct ev_loop *loop_p, ev_signal *w_p, int revents)
{
    LOG4CXX_INFO(g_logger, "Exiting");
    exit(1);
}

