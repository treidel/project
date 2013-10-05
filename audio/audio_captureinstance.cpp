
#include "common.h"
#include "audio_captureinstance.h"
#include "audio_channel.h"
#include "audio_capturemgr.h"
#include "audio_formatter.h"
#include "config.h"

#include <log4cxx/logger.h>

///////////////////////////////////////////////////////////////////////////////
// macros
///////////////////////////////////////////////////////////////////////////////

#define DEFAULT_FULL_SCALE_VOLTAGE      (3.3f)
#define FULLSCALE_VOLTAGE_CONFIG_ITEM   "fullscale-voltage"

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

AUDIOCaptureInstance::AUDIOCaptureInstance(AUDIOCaptureManager *manager_p, const char* device, size_t channel_count, snd_pcm_format_t format, unsigned int rate, snd_pcm_t *handle_p) :
    m_device(strdup(device)),
    m_rate(rate),
    m_handle_p(handle_p),
    m_formatter_p(AUDIOFormatterFactory::create_audio_formatter_p(format)),
    m_channel_count(channel_count),
    m_abort(false)
{
    LOG4CXX_TRACE(g_logger, "AUDIOCaptureInstance::AUDIOCaptureInstance enter " << manager_p << " " << handle_p);

    // resize the vector to hold the number of channels we have
    m_channels.resize(m_channel_count);

    // setup the channels
    for (int counter = 0; counter < m_channel_count; counter++)
    {
        // query for the peak voltage
        std::string channel_section = "channel-" + to_string(counter + 1);
        float voltage = 0.0;
        Config::get_instance_p()->get_float_with_default(channel_section.c_str(), FULLSCALE_VOLTAGE_CONFIG_ITEM, DEFAULT_FULL_SCALE_VOLTAGE, &voltage);

    	// allocate a unique index for the channel
        AUDIOChannel::Index index = manager_p->allocate_index();

        LOG4CXX_INFO(g_logger, "using peak voltage=" + to_string(voltage) + "V for channel=" + to_string(index));

        // create the channel object
        AUDIOChannel *channel_p = new AUDIOChannel(index, rate, voltage);
        // store it in our list
        m_channels[counter] = channel_p;
        // register it with the manager
        manager_p->add_channel(channel_p);
    }

    // launch the processing thread
    if (0 != pthread_create(&m_thread_id, NULL, thread_handler, (void *)this))
    {
        LOG4CXX_ERROR(g_logger, "unable to create thread");
        return;
    }

    LOG4CXX_TRACE(g_logger, "AUDIOCaptureInstance::AUDIOCaptureInstance exit");
}

AUDIOCaptureInstance::~AUDIOCaptureInstance()
{
    LOG4CXX_TRACE(g_logger, "AUDIOCaptureInstance::~AUDIOCaptureInstance enter");

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

    // delete the formatter
    delete m_formatter_p;

    // free the string
    free(m_device);

    LOG4CXX_TRACE(g_logger, "AUDIOCaptureInstance::~AUDIOCaptureInstance exit");
}


///////////////////////////////////////////////////////////////////////////////
// private function implementations
///////////////////////////////////////////////////////////////////////////////

void *AUDIOCaptureInstance::thread_handler(void *arg)
{
    LOG4CXX_TRACE(g_logger, "AUDIOCaptureInstance::thread_handler enter " << arg);

    // get the object instance
    AUDIOCaptureInstance *instance_p = (AUDIOCaptureInstance *)arg;

    // get the channel count locally for convenience
    const size_t channel_count = instance_p->m_channel_count;

    // calculate the number of samples in 50ms of samples 
    const unsigned int num_samples = CALC_NUM_SAMPLES_FOR_MILLIS(50, instance_p->m_rate);

    // allocate a buffer to hold raw audio data
    const snd_pcm_uframes_t buffer_length = num_samples * channel_count * instance_p->m_formatter_p->sample_sizeof();
    uint8_t *raw_buffer_p = (uint8_t *)malloc(buffer_length);

    // allocate buffer to hold the normalized data
    AUDIOChannel::Sample *channel_buffer_p = (AUDIOChannel::Sample *)malloc(num_samples * sizeof(AUDIOChannel::Sample));

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
        rc = snd_pcm_readi(instance_p->m_handle_p, (void *)raw_buffer_p, num_samples);
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
            instance_p->m_formatter_p->format_samples(raw_buffer_p, counter, channel_buffer_p, num_samples);

            // now feed the samples to our consumer e.g. the channel
            AUDIOChannel *channel_p = instance_p->m_channels[counter];
            // write the data to the pipe
            rc = write(channel_p->get_write_fd(), channel_buffer_p, num_samples * sizeof(AUDIOChannel::Sample));
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

    LOG4CXX_TRACE(g_logger, "AUDIOCaptureInstance::thread_handler exit");

    return NULL;
}

