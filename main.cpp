
#include "bluetooth/spp_server.h"
#include "audio/audio_capturemgr.h"

#include <unistd.h>
#include <stdlib.h>
#include <ev.h>

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>

///////////////////////////////////////////////////////////////////////////////
// macros
///////////////////////////////////////////////////////////////////////////////

#define NAME "Leveling Glass"
#define PROCFS_SELF_EXE "/proc/self/exe"

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
static char g_execpath[1024] = {0};
static log4cxx::LoggerPtr g_logger(log4cxx::Logger::getLogger("main"));

///////////////////////////////////////////////////////////////////////////////
// private function declarations
///////////////////////////////////////////////////////////////////////////////

static void kill_cb(struct ev_loop *loop_p, ev_signal *w_p, int revents);
static void populate_execpath(char *execpath_p, size_t path_length);

///////////////////////////////////////////////////////////////////////////////
// public function declarations
///////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
	// use console logging for now
	log4cxx::BasicConfigurator::configure();

	LOG4CXX_INFO(g_logger, "Starting " << NAME << " - version " << VERSION);

	// figure out where our executable is located
	populate_execpath(g_execpath, sizeof g_execpath);

	// create the UUID for our SPP server
 	sdp_uuid128_create(&g_uuid, &c_uuid_int); 
	// start the Bluetooth SPP server
	g_server_p = new SPPServer(g_uuid);

	// create the audio layer
  	AUDIOCaptureManager::getInstance();	

    	// setup the default event loop
    	struct ev_loop *loop_p = EV_DEFAULT;
    	// register a signal handler for the KILL event
    	ev_signal kill_watcher;
    	ev_signal_init(&kill_watcher, kill_cb, SIGKILL);
    	ev_signal_start(loop_p, &kill_watcher);

	// run forever
    	ev_run(loop_p, 0);   

	// we should never get here
	LOG4CXX_FATAL(g_logger, "main loop exited");
    	return -1;
}

void kill_cb(struct ev_loop *loop_p, ev_signal *w_p, int revents)
{
	LOG4CXX_INFO(g_logger, "Exiting");
	exit(1);
}

void populate_execpath(char *path_p, size_t path_length)
{
	ssize_t data_length = readlink(PROCFS_SELF_EXE, path_p, path_length);
	ASSERT(data_length < path_length);
	path_p[data_length] = 0;
}
