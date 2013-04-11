
// this line is required to get the limit macros
#define __STDC_LIMIT_MACROS

#include "audio_processor.h"
#include "audio_capturemgr.h"

#include <ev.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdint.h>

#include <log4cxx/logger.h>

///////////////////////////////////////////////////////////////////////////////
// macros
///////////////////////////////////////////////////////////////////////////////

#define UPDATES_PER_SECOND (10)

///////////////////////////////////////////////////////////////////////////////
// type defintions
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// constants
///////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////
// module variables
///////////////////////////////////////////////////////////////////////////////
static log4cxx::LoggerPtr g_logger(log4cxx::Logger::getLogger("audio.processor"));

///////////////////////////////////////////////////////////////////////////////
// private function declarations
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// public function implementations
///////////////////////////////////////////////////////////////////////////////

AUDIOProcessor::AUDIOProcessor(Handler *handler_p) :
	m_handler_p(handler_p),
	m_loop_p(ev_default_loop(0))
{
	LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::AUDIOProcessor enter " << handler_p);

	// register for audio data
	AUDIOCaptureManager::getInstance()->addHandler(this);

    	// initialize the channel data 
    	size_t channel_count = AUDIOCaptureManager::getInstance()->channel_count();
	m_channel_data = new Data[channel_count];

    	// setup the periodic timer 
    	ev_timer_init(&m_timer, timer_cb, 1.0/UPDATES_PER_SECOND, 0.0);
    	ev_timer_start(m_loop_p, &m_timer); 

	LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::AUDIOProcessor exit");
}

AUDIOProcessor::~AUDIOProcessor() 
{
	LOG4CXX_DEBUG(g_logger, "AUDIOProcsesor::~AUDIOProcessor enter");

	// deregister
	AUDIOCaptureManager::getInstance()->removeHandler(this);

	// release our data
	delete [] m_channel_data;
	
	// stop the timer
	ev_timer_stop(m_loop_p, &m_timer);

	LOG4CXX_DEBUG(g_logger, "AUDIOProcsesor::~AUDIOProcessor exit");
}


///////////////////////////////////////////////////////////////////////////////
// public function implementations
///////////////////////////////////////////////////////////////////////////////

ResultCode AUDIOProcessor::handle_samples(AUDIOChannel::Index index, const size_t buffer_length, AUDIOChannel::Sample *buffer_p)
{
	LOG4CXX_DEBUG(g_logger, "AUDIOProcsesor::handler_samples enter " << index << " " << buffer_length << " " << buffer_p);

	// range check input
	ASSERT((index > 0) && (index <= sizeof m_channel_data));

    	// get the channel data
    	Data &data = m_channel_data[index - 1];
   
    	// calculate the peak amplitude for the signal
    	for (int counter = 0; counter < buffer_length; counter++)
    	{
		// take the absolute value
		AUDIOChannel::Sample amplitude = abs(buffer_p[counter]);
       		// if this sample is larger then last one calculated for the channel then 
       		// store it 
      		if (amplitude > data.peak)
       		{
           		data.peak = amplitude;   
       		}
    	}

	LOG4CXX_DEBUG(g_logger, "AUDIOProcsesor::handler_samples exit");

    	return RESULT_CODE_OK;
}

///////////////////////////////////////////////////////////////////////////////
// private function implementations
///////////////////////////////////////////////////////////////////////////////

void AUDIOProcessor::timer_cb(EV_P_ ev_timer *w_p, int revents)
{
	LOG4CXX_DEBUG(g_logger, "AUDIOProcsesor::timer_cb enter " << w_p << " " << revents);

	// get the object
	AUDIOProcessor *processor_p = (AUDIOProcessor *)w_p->data;
	// call the handler
	processor_p->m_handler_p->handle_results(processor_p->m_channel_data);
    	// clear each channel
    	for (int counter = 0; counter < sizeof m_channel_data; counter++)
    	{
       		Data &data = processor_p->m_channel_data[counter];
		memset(&data, 0, sizeof(Data));
        } 

	LOG4CXX_DEBUG(g_logger, "AUDIOProcsesor::timer_cb exit");
}
