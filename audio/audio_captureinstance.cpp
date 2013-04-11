
#include "common.h"
#include "audio_captureinstance.h"
#include "audio_channel.h"
#include "audio_capturemgr.h"

#include <log4cxx/logger.h>

///////////////////////////////////////////////////////////////////////////////
// macros
///////////////////////////////////////////////////////////////////////////////

#define SAMPLE_RATE_IN_HZ (41000)
#define CHANNEL_COUNT (2)
#define BUFFER_SIZE_IN_SAMPLES (128)

///////////////////////////////////////////////////////////////////////////////
// type defintions
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// constants
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// module variables
///////////////////////////////////////////////////////////////////////////////

static log4cxx::LoggerPtr g_logger(log4cxx::Logger::getLogger("audio.captureinstance"));


///////////////////////////////////////////////////////////////////////////////
// private function declarations
///////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////
// public function implementations
///////////////////////////////////////////////////////////////////////////////

AUDIOCaptureInstance::AUDIOCaptureInstance(AUDIOCaptureManager *manager_p, const char *device) :
	m_handle_p(NULL),
	m_channel_count(CHANNEL_COUNT),
	m_abort(false)
{
	LOG4CXX_DEBUG(g_logger, "AUDIOCaptureInstance::AUDIOCaptureInstance enter " << manager_p << " " << device);

	// allocate the channel array
	m_channels_pp = new AUDIOChannel *[m_channel_count];

    	// setup the HW params pointer so that we can free it in case of errors
    	snd_pcm_hw_params_t *hw_params_p = NULL;
    	unsigned int rate = SAMPLE_RATE_IN_HZ;

    	// open the sound device
    	int rc = snd_pcm_open(&m_handle_p, device, SND_PCM_STREAM_CAPTURE, SND_PCM_CLASS_GENERIC);
	if (rc < 0)
    	{
		LOG4CXX_ERROR(g_logger, "snd_pcm_open returned error=" << rc);
		return;
    	}
		 
    	// allocate the hardware params data structure
    	rc = snd_pcm_hw_params_malloc(&hw_params_p);
	if (rc < 0)
    	{
		LOG4CXX_ERROR(g_logger, "snd_pcm_hw_params_malloc returned error=" << rc);
		return;
    	}

    	// initialize the default hardware params
    	rc = snd_pcm_hw_params_any(m_handle_p, hw_params_p);
	if (rc < 0)
    	{
		LOG4CXX_ERROR(g_logger, "snd_pcm_hw_params_any returned error=" << rc);
		return;
    	}
	
    	// specify that we want interleaved data
    	rc = snd_pcm_hw_params_set_access(m_handle_p, hw_params_p, SND_PCM_ACCESS_RW_INTERLEAVED);
	if (rc < 0)
    	{
		LOG4CXX_ERROR(g_logger, "snd_pcm_hw_params_set_access returned error=" << rc);
		return;
    	}
       
	// set the format of the audio samples 
    	rc = snd_pcm_hw_params_set_format(m_handle_p, hw_params_p, SND_PCM_FORMAT_FLOAT_LE);
	if (rc < 0)
    	{
		LOG4CXX_ERROR(g_logger, "snd_pcm_hw_params_set_format returned error=" << rc);
		return;
	}
        
	// set the sample rate
    	rc = snd_pcm_hw_params_set_rate_near(m_handle_p, hw_params_p, &rate, NULL);
	if (rc < 0)
    	{
		LOG4CXX_ERROR(g_logger, "snd_pcm_hw_params_set_rate_near returned error=" << rc);
		return;
    	}	
	
	// set the number of channels
    	rc = snd_pcm_hw_params_set_channels(m_handle_p, hw_params_p, CHANNEL_COUNT);
	if (rc < 0)
    	{
		LOG4CXX_ERROR(g_logger, "snd_pcm_hw_params_set_channels returned error=" << rc);
		return;
    	}

	// set the hardware params 
    	if(0 > snd_pcm_hw_params(m_handle_p, hw_params_p))
    	{
		LOG4CXX_ERROR(g_logger, "snd_pcm_hw_params returned error=" << rc);
		return;
    	}

    	// setup the channels
    	for (int counter = 0; counter < m_channel_count; counter++)
    	{
		AUDIOChannel::Index index = manager_p->allocate_index();
		AUDIOChannel *channel_p = new AUDIOChannel(index);
		m_channels_pp[counter] = channel_p;
	}

        // launch the processing thread
        if (0 != pthread_create(&m_thread_id, NULL, thread_handler, (void *)this))
        {
		LOG4CXX_ERROR(g_logger, "unable to create thread");
		return;
  	}
        
    	// free the sound params in all cases
    	snd_pcm_hw_params_free(hw_params_p);

	LOG4CXX_DEBUG(g_logger, "AUDIOCaptureInstance::AUDIOCaptureInstance exit");
}

AUDIOCaptureInstance::~AUDIOCaptureInstance()
{
	LOG4CXX_DEBUG(g_logger, "AUDIOCaptureInstance::~AUDIOCaptureInstance enter");

	// tell the thread to abort
	m_abort = true;
	
	// wait for the thread to exit
	pthread_join(m_thread_id, NULL);
	
	// close the sound handle
        snd_pcm_close(m_handle_p);

	// delete the channels
	for (int counter = 0; counter < m_channel_count; counter++)
	{
		delete m_channels_pp[counter];
	}
	delete [] m_channels_pp;

	LOG4CXX_DEBUG(g_logger, "AUDIOCaptureInstance::~AUDIOCaptureInstance exit");
}
    

///////////////////////////////////////////////////////////////////////////////
// private function implementations
///////////////////////////////////////////////////////////////////////////////

void *AUDIOCaptureInstance::thread_handler(void *arg)
{
	LOG4CXX_DEBUG(g_logger, "AUDIOCaptureInstance::thread_handler enter " << arg);

	AUDIOCaptureInstance *instance_p = (AUDIOCaptureInstance *)arg;

    	// tell the audio device we want some data now please
    	int rc = snd_pcm_prepare(instance_p->m_handle_p);
	if (rc < 0)
    	{
		LOG4CXX_ERROR(g_logger, "snd_pcm_prepare returned error=" << rc);
        	return NULL;
    	}

    	// allocate a buffer to hold audio data
    	const snd_pcm_uframes_t buffer_length = BUFFER_SIZE_IN_SAMPLES * CHANNEL_COUNT * sizeof(AUDIOChannel::Sample);
    	AUDIOChannel::Sample *buffer_p = (AUDIOChannel::Sample *)malloc(buffer_length);

    	// allocate buffers to hold the de-interlaced data
    	AUDIOChannel::Sample *channel_buffer_pp[instance_p->m_channel_count];
    	for (int counter = 0; counter < instance_p->m_channel_count; counter++)
    	{
      		channel_buffer_pp[counter] = (AUDIOChannel::Sample *)malloc(BUFFER_SIZE_IN_SAMPLES * sizeof(AUDIOChannel::Sample));
    	}

    	// debug counter to see how many samples we've captured
    	int samples = 0;

    	// loop until signalled
    	while (false == instance_p->m_abort)
    	{
		LOG4CXX_DEBUG(g_logger, "AUDIOCaptureInstance::thread_handler samples " << samples);

        	// read some data
        	rc = snd_pcm_readi(instance_p->m_handle_p, (void *)buffer_p, buffer_length);
        	if (-EPIPE == rc) 
        	{
            		// EPIPE is returned when we were too slow in retrieving a sample

			LOG4CXX_WARN(g_logger, "received EPIPE, buffer underrun capturing samples");

            		// reinitialize the channel
            		rc = snd_pcm_prepare(instance_p->m_handle_p);
			if (rc < 0)
            		{
				LOG4CXX_ERROR(g_logger, "snd_pcm_prepare returned error=" << rc);
                		goto error;
            		}
            		continue;
        	}
        	else if (0 > rc)
        	{ 
			LOG4CXX_ERROR(g_logger, "snd_pcm_readi returned error=" << rc);
            		goto error;
        	}

        	// we successfully collected a sample
        	samples++;

        	// de-interlace the samples
        	for(int channel_counter = 0; channel_counter < instance_p->m_channel_count; channel_counter++)
        	{
            		for(int sample_counter = 0; sample_counter < BUFFER_SIZE_IN_SAMPLES; sample_counter++)
            		{
              			channel_buffer_pp[channel_counter][sample_counter] = buffer_p[(CHANNEL_COUNT * sample_counter) + channel_counter];
            		}
        	}
        	// now feed the data to our consumers
        	for (int counter = 0; counter < CHANNEL_COUNT; counter++)
        	{
            		// get the channel
            		AUDIOChannel *channel_p = instance_p->m_channels_pp[counter];
            		// write the data to the pipe
            		rc = write(channel_p->getWriteFD(), channel_buffer_pp[counter], BUFFER_SIZE_IN_SAMPLES * sizeof(AUDIOChannel::Sample));
			if (rc < 0)
            		{
				LOG4CXX_ERROR(g_logger, "write returned error=" << rc)
                 		goto error;          
            		}
        	}
    	}

error:
    	// free the interlaced buffer
    	free(buffer_p);

    	// free the de-interlaced buffers
    	for (int counter = 0; counter < CHANNEL_COUNT; counter++)
    	{
        	free(channel_buffer_pp[counter]);
   	}

	LOG4CXX_DEBUG(g_logger, "AUDIOCaptureInstance::thread_handler exit");

	return NULL;
}

