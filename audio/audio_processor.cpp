
// this line is required to get the limit macros
#define __STDC_LIMIT_MACROS

#include "audio_processor.h"
#include "audio_capturemgr.h"

#include <ev.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdint.h>

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
	// register for audio data
	AUDIOCaptureManager::getInstance()->addHandler(this);

    	// initialize the channel data 
    	size_t channel_count = AUDIOCaptureManager::getInstance()->channel_count();
	m_channel_data = new Data[channel_count];

    	// setup the periodic timer 
    	ev_timer_init(&m_timer, timer_cb, 1.0/UPDATES_PER_SECOND, 0.0);
    	ev_timer_start(m_loop_p, &m_timer); 
}

AUDIOProcessor::~AUDIOProcessor() 
{
	// deregister
	AUDIOCaptureManager::getInstance()->removeHandler(this);

	// release our data
	delete [] m_channel_data;
	
	// stop the timer
	ev_timer_stop(m_loop_p, &m_timer);
}


///////////////////////////////////////////////////////////////////////////////
// public function implementations
///////////////////////////////////////////////////////////////////////////////

ResultCode AUDIOProcessor::handle_samples(AUDIOChannel::Index index, const size_t buffer_length, AUDIOChannel::Sample *buffer_p)
{
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

    	return RESULT_CODE_OK;
}

///////////////////////////////////////////////////////////////////////////////
// private function implementations
///////////////////////////////////////////////////////////////////////////////

void AUDIOProcessor::timer_cb(EV_P_ ev_timer *w, int revents)
{
	// get the object
	AUDIOProcessor *processor_p = (AUDIOProcessor *)w->data;
	// call the handler
	processor_p->m_handler_p->handle_results(processor_p->m_channel_data);
    	// clear each channel
    	for (int counter = 0; counter < sizeof m_channel_data; counter++)
    	{
       		Data &data = processor_p->m_channel_data[counter];
		memset(&data, 0, sizeof(Data));
        } 
}
