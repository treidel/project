
#include "common.h"
#include "audio_captureinstance.h"
#include "audio_channel.h"
#include "audio_capturemgr.h"
#include "audio_formatter.h"

#include <log4cxx/logger.h>

///////////////////////////////////////////////////////////////////////////////
// macros
///////////////////////////////////////////////////////////////////////////////

#define SAMPLE_RATE_IN_HZ (41000)

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
	m_device(strdup(device)),
	m_handle_p(NULL),
	m_formatter_p(NULL),
	m_channel_count(0),
	m_abort(false)
{
	LOG4CXX_DEBUG(g_logger, "AUDIOCaptureInstance::AUDIOCaptureInstance enter " << manager_p << " " << device);

    	// setup the HW params pointer so that we can free it in case of errors
    	snd_pcm_hw_params_t *hw_params_p = NULL;
    	unsigned int rate = SAMPLE_RATE_IN_HZ;

    	// open the sound device
    	int rc = snd_pcm_open(&m_handle_p, device, SND_PCM_STREAM_CAPTURE, SND_PCM_CLASS_GENERIC);
	if (rc < 0)
    	{
		LOG4CXX_ERROR(g_logger, "snd_pcm_open returned error=" << rc << " " << snd_strerror(rc));
		return;
    	}

    	// allocate the hardware params data structure
    	rc = snd_pcm_hw_params_malloc(&hw_params_p);
	if (rc < 0)
    	{
		LOG4CXX_ERROR(g_logger, "snd_pcm_hw_params_malloc returned error=" << rc << " " << snd_strerror(rc));
		return;
    	}

    	// initialize the default hardware params
    	rc = snd_pcm_hw_params_any(m_handle_p, hw_params_p);
	if (rc < 0)
    	{
		LOG4CXX_ERROR(g_logger, "snd_pcm_hw_params_any returned error=" << rc << " " << snd_strerror(rc));
		return;
    	}

	// read the number channels
	rc = snd_pcm_hw_params_get_channels_max(hw_params_p, &m_channel_count);
	if (rc < 0)
    	{
		LOG4CXX_ERROR(g_logger, "snd_pcm_hw_params_get_channels_max returned error=" << rc << " " << snd_strerror(rc));
		return;
    	}

    	// specify that we want non-interleaved data
    	rc = snd_pcm_hw_params_set_access(m_handle_p, hw_params_p, SND_PCM_ACCESS_RW_INTERLEAVED);
	if (rc < 0)
    	{
		LOG4CXX_ERROR(g_logger, "snd_pcm_hw_params_set_access returned error=" << rc << " " << snd_strerror(rc));
		return;
    	}

	// query the list of supported formats
	snd_pcm_format_mask_t *format_mask_p = NULL;
	rc = snd_pcm_format_mask_malloc(&format_mask_p);
	if (0 > rc)
	{
		LOG4CXX_ERROR(g_logger, "snd_pcm_format_mask_malloc returned error=" << rc << " " << snd_strerror(rc));
		return;
 
	}
	snd_pcm_hw_params_get_format_mask(hw_params_p, format_mask_p);
	// check for our preferred format
	if (0 != snd_pcm_format_mask_test(format_mask_p, SND_PCM_FORMAT_FLOAT_LE))
	{
		m_formatter_p = AUDIOFormatterFactory::createAudioFormatter_p(SND_PCM_FORMAT_FLOAT_LE);
	}
	else if (0 != snd_pcm_format_mask_test(format_mask_p, SND_PCM_FORMAT_S16_LE))
	{
		m_formatter_p = AUDIOFormatterFactory::createAudioFormatter_p(SND_PCM_FORMAT_S16_LE);
	}
	else
	{
		LOG4CXX_ERROR(g_logger, "no supported sample formats for " << device);
		return;
	}
	snd_pcm_format_mask_free(format_mask_p);

	// set the format of the audio samples 
    	rc = snd_pcm_hw_params_set_format(m_handle_p, hw_params_p, m_formatter_p->format());
	if (rc < 0)
    	{
		LOG4CXX_ERROR(g_logger, "snd_pcm_hw_params_set_format returned error=" << rc << " " << snd_strerror(rc));
		return;
	}
        
	// set the sample rate
    	rc = snd_pcm_hw_params_set_rate_near(m_handle_p, hw_params_p, &rate, NULL);
	if (rc < 0)
    	{
		LOG4CXX_ERROR(g_logger, "snd_pcm_hw_params_set_rate_near returned error=" << rc << " " << snd_strerror(rc));
		return;
    	}	
	
	// set the number of channels
    	rc = snd_pcm_hw_params_set_channels(m_handle_p, hw_params_p, m_channel_count);
	if (rc < 0)
    	{
		LOG4CXX_ERROR(g_logger, "snd_pcm_hw_params_set_channels returned error=" << rc << " " << snd_strerror(rc));
		return;
    	}

	// set the hardware params 
    	if(0 > snd_pcm_hw_params(m_handle_p, hw_params_p))
    	{
		LOG4CXX_ERROR(g_logger, "snd_pcm_hw_params returned error=" << rc << " " << snd_strerror(rc));
		return;
    	}

    	// setup the channels
    	for (int counter = 0; counter < m_channel_count; counter++)
    	{
		AUDIOChannel::Index index = manager_p->allocate_index();
		AUDIOChannel *channel_p = new AUDIOChannel(index, SAMPLE_RATE_IN_HZ);
		m_channels[counter] = channel_p;
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
	for (std::vector<AUDIOChannel *>::iterator it = m_channels.begin(); 
             it != m_channels.end();
             it++)
	{
		delete *it;
	}

	// free the device name
	delete m_device;

	// delete the formatter
	delete m_formatter_p;

	// free the string
	free(m_device);

	LOG4CXX_DEBUG(g_logger, "AUDIOCaptureInstance::~AUDIOCaptureInstance exit");
}
    

///////////////////////////////////////////////////////////////////////////////
// private function implementations
///////////////////////////////////////////////////////////////////////////////

void *AUDIOCaptureInstance::thread_handler(void *arg)
{
	LOG4CXX_DEBUG(g_logger, "AUDIOCaptureInstance::thread_handler enter " << arg);

	// get the object instance
	AUDIOCaptureInstance *instance_p = (AUDIOCaptureInstance *)arg; 

	// get the channel count locally for convenience 
	const size_t channel_count = instance_p->m_channel_count;

    	// allocate a buffer to hold raw audio data
    	const snd_pcm_uframes_t buffer_length = BUFFER_SIZE_IN_SAMPLES * channel_count * instance_p->m_formatter_p->sample_sizeof();
    	uint8_t *raw_buffer_p = (uint8_t *)malloc(buffer_length);

    	// allocate buffer to hold the normalized data
    	AUDIOChannel::Sample *channel_buffer_p = (AUDIOChannel::Sample *)malloc(BUFFER_SIZE_IN_SAMPLES * sizeof(AUDIOChannel::Sample));

  	// tell the audio device we want some data now please
    	int rc = snd_pcm_prepare(instance_p->m_handle_p);
	if (rc < 0)
    	{
		LOG4CXX_ERROR(g_logger, "snd_pcm_prepare returned error=" << rc << " " << snd_strerror(rc));
        	return NULL;
    	}

	LOG4CXX_INFO(g_logger, "Collection started for " << instance_p->m_device);

    	// debug counter to see how many samples we've captured
    	int samples = 0;

    	// loop until signalled
    	while (false == instance_p->m_abort)
    	{
        	// read some data
        	rc = snd_pcm_readi(instance_p->m_handle_p, (void *)raw_buffer_p, BUFFER_SIZE_IN_SAMPLES);
        	if (-EPIPE == rc) 
        	{
            		// EPIPE is returned when we were too slow in retrieving a sample

			LOG4CXX_WARN(g_logger, "received EPIPE, buffer underrun capturing samples");

            		// reinitialize the channel
            		rc = snd_pcm_prepare(instance_p->m_handle_p);
			if (rc < 0)
            		{
				LOG4CXX_ERROR(g_logger, "snd_pcm_prepare returned error=" << rc << " " << snd_strerror(rc));
                		goto error;
            		}
            		continue;
        	}
        	else if (0 > rc)
        	{ 
			LOG4CXX_ERROR(g_logger, "snd_pcm_readi returned error=" << rc << " " << snd_strerror(rc));
            		goto error;
        	}

        	// we successfully collected a sample
        	samples++;

        	// go through and handle the audio data for each channel
		for (int counter = 0; counter < channel_count; counter++)
		{ 
			// let the formatter de-interlace convert the samples for us 
			instance_p->m_formatter_p->format_samples(raw_buffer_p, counter, channel_buffer_p, BUFFER_SIZE_IN_SAMPLES);

        		// now feed the samples to our consumer e.g. the channel
            		AUDIOChannel *channel_p = instance_p->m_channels[counter];
            		// write the data to the pipe
            		rc = write(channel_p->get_write_fd(), channel_buffer_p, BUFFER_SIZE_IN_SAMPLES * sizeof(AUDIOChannel::Sample));
			if (rc < 0)
            		{
				LOG4CXX_ERROR(g_logger, "write returned error=" << rc)
                 		goto error;          
            		}
        	}
    	}

error:
    	// free the raw buffer
    	free(raw_buffer_p);

	// free the channel buffer
        free(channel_buffer_p);

	LOG4CXX_DEBUG(g_logger, "AUDIOCaptureInstance::thread_handler exit");

	return NULL;
}

