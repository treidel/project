
#include "common.h"
#include "audio_capturemgr.h"
#include "audio_captureinstance.h"
#include "audio_channel.h"

#include <ev.h>
#include <stdlib.h>
#include <unistd.h>

#include <log4cxx/logger.h>

///////////////////////////////////////////////////////////////////////////////
// macros
///////////////////////////////////////////////////////////////////////////////

#define CAPTURE_DEVICE "hw:0,0"

///////////////////////////////////////////////////////////////////////////////
// type defintions
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// constants
///////////////////////////////////////////////////////////////////////////////

#define BUFFER_SIZE_IN_SAMPLES (128)
#define BUFFER_SIZE_IN_BYTES (BUFFER_SIZE_IN_SAMPLES * sizeof(AUDIOChannel::Sample))

///////////////////////////////////////////////////////////////////////////////
// module variables
///////////////////////////////////////////////////////////////////////////////

static log4cxx::LoggerPtr g_logger(log4cxx::Logger::getLogger("audio.capturemgr"));

// singleton pointer
AUDIOCaptureManager *AUDIOCaptureManager::g_instance_p = NULL;

// we get away with having a single buffer by virtue of being single threaded
// in event loop
static AUDIOChannel::Sample *g_buffer_p = (AUDIOChannel::Sample *)malloc(BUFFER_SIZE_IN_BYTES);

///////////////////////////////////////////////////////////////////////////////
// private function declarations
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// public function implementations
///////////////////////////////////////////////////////////////////////////////

AUDIOCaptureManager *AUDIOCaptureManager::getInstance()
{
	LOG4CXX_DEBUG(g_logger, "AUDIOCaptureManager::getInstance enter");

	if (NULL == g_instance_p)
	{
		g_instance_p = new AUDIOCaptureManager();
	}

	LOG4CXX_DEBUG(g_logger, "AUDIOCaptureManager::getInstance exit " << g_instance_p);

	return g_instance_p; 
}

AUDIOCaptureManager::~AUDIOCaptureManager()
{
	LOG4CXX_DEBUG(g_logger, "AUDIOCaptureManager::~AUDIOCaptureManager enter");

	for (int index = 0; index < sizeof m_instances; index++)
	{
		delete m_instances[index];
	}	
	delete [] m_instances;

	LOG4CXX_DEBUG(g_logger, "AUDIOCaptureManager::~AUDIOCaptureManager enter");
}

void AUDIOCaptureManager::addHandler(Handler *handler_p)
{
	LOG4CXX_DEBUG(g_logger, "AUDIOCaptureManager::addHandler enter " << handler_p);

	m_handlers.push_front(handler_p);

	LOG4CXX_DEBUG(g_logger, "AUDIOCaptureManager::addHandler exit");
}

void AUDIOCaptureManager::removeHandler(Handler *handler_p)
{
	LOG4CXX_DEBUG(g_logger, "AUDIOCaptureManager::removeHandler enter " << handler_p);

	m_handlers.remove(handler_p);

	LOG4CXX_DEBUG(g_logger, "AUDIOCaptureManager::removeHandler exit");
}



///////////////////////////////////////////////////////////////////////////////
// private function implementations
///////////////////////////////////////////////////////////////////////////////

AUDIOCaptureManager::AUDIOCaptureManager() :
	m_channel_count(0),
	m_loop_p(ev_default_loop(0))
	
{
	LOG4CXX_DEBUG(g_logger, "AUDIOCaptureManager::AUDIOCaptureManager enter");
	
	// TBD: dynamically discover the available audio capture devices

	// allocate the array for the capture instances
	m_instances = new AUDIOCaptureInstance *[1];
	m_instances[0] = new AUDIOCaptureInstance(this, CAPTURE_DEVICE);

	LOG4CXX_DEBUG(g_logger, "AUDIOCaptureManager:AUDIOCaptureManager exit");
}

AUDIOChannel::Index AUDIOCaptureManager::allocate_index() 
{
	LOG4CXX_DEBUG(g_logger, "AUDIOCaptureManager::allocate_index enter");

	m_channel_count++;
	AUDIOChannel::Index index = (AUDIOChannel::Index)m_channel_count;

	LOG4CXX_DEBUG(g_logger, "AUDIOCaptureManager::allocate_index exit " << index);
	return index;
}


