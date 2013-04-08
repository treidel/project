
#include "common.h"
#include "audio_capturemgr.h"
#include "audio_captureinstance.h"
#include "audio_channel.h"

#include <ev.h>
#include <stdlib.h>
#include <unistd.h>

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
	if (NULL == g_instance_p)
	{
		g_instance_p = new AUDIOCaptureManager();
	}
	return g_instance_p; 
}

AUDIOCaptureManager::~AUDIOCaptureManager()
{
	
	for (int index = 0; index < sizeof m_instances; index++)
	{
		delete m_instances[index];
	}	
	delete [] m_instances;
}

void AUDIOCaptureManager::addHandler(Handler *handler_p)
{
	m_handlers.push_front(handler_p);
}

void AUDIOCaptureManager::removeHandler(Handler *handler_p)
{
	m_handlers.remove(handler_p);
}



///////////////////////////////////////////////////////////////////////////////
// private function implementations
///////////////////////////////////////////////////////////////////////////////

AUDIOCaptureManager::AUDIOCaptureManager() :
	m_channel_count(0),
	m_loop_p(ev_default_loop(0))
	
{
	// TBD: dynamically discover the available audio capture devices

	// allocate the array for the capture instances
	m_instances = new AUDIOCaptureInstance *[1];
	m_instances[0] = new AUDIOCaptureInstance(this, CAPTURE_DEVICE);
}

AUDIOChannel::Index AUDIOCaptureManager::allocate_index() 
{
	m_channel_count++;
	return (AUDIOChannel::Index)m_channel_count;
}


