
#include "common.h"
#include "audio_capturemgr.h"
#include "audio_captureinstance.h"
#include "audio_channel.h"
#include "audio_formatter.h"

#include <ev.h>
#include <stdlib.h>
#include <unistd.h>

#include <log4cxx/logger.h>

///////////////////////////////////////////////////////////////////////////////
// macros
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// type defintions
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// constants
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// module variables
///////////////////////////////////////////////////////////////////////////////

static log4cxx::LoggerPtr g_logger(log4cxx::Logger::getLogger("audio.capturemgr"));

// singleton pointer
AUDIOCaptureManager *AUDIOCaptureManager::g_instance_p = NULL;

///////////////////////////////////////////////////////////////////////////////
// private function declarations
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// public function implementations
///////////////////////////////////////////////////////////////////////////////

AUDIOCaptureManager *AUDIOCaptureManager::get_instance()
{
    LOG4CXX_TRACE(g_logger, "AUDIOCaptureManager::get_instance enter");

    if (NULL == g_instance_p)
    {
        g_instance_p = new AUDIOCaptureManager();
    }

    LOG4CXX_TRACE(g_logger, "AUDIOCaptureManager::get_instance exit " << g_instance_p);

    return g_instance_p;
}

AUDIOCaptureManager::~AUDIOCaptureManager()
{
    LOG4CXX_TRACE(g_logger, "AUDIOCaptureManager::~AUDIOCaptureManager enter");

    for (std::list<AUDIOCaptureInstance *>::iterator it = m_instances.begin();
            it != m_instances.end();
            it++)
    {
        delete *it;
    }
    g_instance_p = NULL;

    LOG4CXX_TRACE(g_logger, "AUDIOCaptureManager::~AUDIOCaptureManager enter");
}

void AUDIOCaptureManager::add_handler(Handler *handler_p)
{
    LOG4CXX_TRACE(g_logger, "AUDIOCaptureManager::add_handler enter " << handler_p);

    m_handlers.push_front(handler_p);

    LOG4CXX_TRACE(g_logger, "AUDIOCaptureManager::add_handler exit");
}

void AUDIOCaptureManager::remove_handler(Handler *handler_p)
{
    LOG4CXX_TRACE(g_logger, "AUDIOCaptureManager::remove_handler enter " << handler_p);

    m_handlers.remove(handler_p);

    LOG4CXX_TRACE(g_logger, "AUDIOCaptureManager::remove_handler exit");
}

AUDIOChannel *AUDIOCaptureManager::find_channel(const AUDIOChannel::Index index)
{
    LOG4CXX_TRACE(g_logger, "AUDIOCaptureManager::find_channel enter " << index);
    // assume we won't find it 
    AUDIOChannel *channel_p = NULL;
    // if we have a meter let it do its thing
    std::map<AUDIOChannel::Index, AUDIOChannel *>::iterator it = m_channels_map.find(index);
    if (m_channels_map.end() != it)
    {
        channel_p = it->second;
    }

    LOG4CXX_TRACE(g_logger, "AUDIOCaptureManager::find_channel exit channel_p=" << channel_p);
    return channel_p;
}

void AUDIOCaptureManager::add_channel(AUDIOChannel *channel_p)
{
    LOG4CXX_TRACE(g_logger, "AUDIOCaptureManager::add_channel enter " << channel_p);

    m_channels_map[channel_p->get_index()] = channel_p;

    LOG4CXX_TRACE(g_logger, "AUDIOCaptureManager::add_channel exit");

}

///////////////////////////////////////////////////////////////////////////////
// private function implementations
///////////////////////////////////////////////////////////////////////////////

AUDIOCaptureManager::AUDIOCaptureManager() :
    m_channel_count(0),
    m_loop_p(ev_default_loop(0))

{
    LOG4CXX_TRACE(g_logger, "AUDIOCaptureManager::AUDIOCaptureManager enter");

    // query the list of formats we support
    std::list<snd_pcm_format_t> format_list = AUDIOFormatterFactory::fetch_audio_format_list();

    // iterate for all cards
    int card_index = -1;
    for (int rc_card = snd_card_next(&card_index);
         (0 == rc_card) && (0 <= card_index);
         rc_card = snd_card_next(&card_index))
    {
        LOG4CXX_TRACE(g_logger, "found card=" + to_string(card_index));

        char card[3 + 10 + 1];
        sprintf(card, "hw:%d", card_index);

        snd_ctl_t *card_handle_p = NULL;
        if (0 > snd_ctl_open(&card_handle_p, card, 0))
        {
            LOG4CXX_ERROR(g_logger, "error opening card=" + to_string(card_index));
            return;
        }
        // iterate for all devices
        int device_index = -1;
        for (int rc_device = snd_ctl_pcm_next_device(card_handle_p, &device_index);
                (0 == rc_device) && (0 <= device_index);
                rc_device = snd_ctl_pcm_next_device(card_handle_p, &device_index))
        {
            LOG4CXX_TRACE(g_logger, "found device=" + to_string(card_index) + "," + to_string(device_index));

            char device[3 + 10 + 1 + 10 + 1];
            sprintf(device, "hw:%d,%d", card_index, device_index);
            // try to open the sound device
            snd_pcm_t *device_handle_p = NULL;
            int rc = snd_pcm_open(&device_handle_p, device, SND_PCM_STREAM_CAPTURE, SND_PCM_CLASS_GENERIC);
            if (0 > rc)
            {
                LOG4CXX_TRACE(g_logger, "snd_pcm_open returned error=" + to_string(rc) + " " + snd_strerror(rc) + " for device=" + device);
                continue;
            }
            
            // query the hardware params
            snd_pcm_hw_params_t *hw_params_p = NULL;
            // allocate the hardware params data structure
            snd_pcm_hw_params_malloc(&hw_params_p);

            // initialize the default hardware params
            rc = snd_pcm_hw_params_any(device_handle_p, hw_params_p);
            if (0 > rc)
            {
                LOG4CXX_ERROR(g_logger, "snd_pcm_hw_params_any returned error=" << rc << " " << snd_strerror(rc));
                return;
            }
            
            // iterate for each supported list
            snd_pcm_format_t format = SND_PCM_FORMAT_UNKNOWN;
            for (std::list<snd_pcm_format_t>::iterator iterator = format_list.begin();
                 iterator != format_list.end();
                 ++iterator)
            {
                if (0 == snd_pcm_hw_params_test_format(device_handle_p, hw_params_p, *iterator))
                {
                    format = *iterator;
                    break;
                }
            }

            // check if we found a supported format
            if (SND_PCM_FORMAT_UNKNOWN == format)
            {
                LOG4CXX_TRACE(g_logger, "no supported audio formats found for device=" << device);
                // close the device
                snd_pcm_close(device_handle_p);
                // release the memory
                snd_pcm_hw_params_free(hw_params_p);
                // go to the next device
                continue;
            }

             // set the format of the audio samples
            rc = snd_pcm_hw_params_set_format(device_handle_p, hw_params_p, format);
            if (rc < 0)
            {
                LOG4CXX_ERROR(g_logger, "snd_pcm_hw_params_set_format returned error=" << rc << " " << snd_strerror(rc));
                return;
            }

            // read the number channels
            size_t channel_count = 0;
            rc = snd_pcm_hw_params_get_channels_max(hw_params_p, &channel_count);
            if (rc < 0)
            {
                LOG4CXX_ERROR(g_logger, "snd_pcm_hw_params_get_channels_max returned error=" << rc << " " << snd_strerror(rc));
                return;
            }

            // we want all of the channels
            rc = snd_pcm_hw_params_set_channels(device_handle_p, hw_params_p, channel_count);
            if (rc < 0)
            {
                LOG4CXX_ERROR(g_logger, "snd_pcm_hw_params_set_channels returned error=" << rc << " " << snd_strerror(rc));
                return;
            }

            // query the maximum sample rate
            unsigned int rate = 0;
            rc = snd_pcm_hw_params_get_rate_max(hw_params_p, &rate, NULL);
            if (rc < 0)
            {
                LOG4CXX_ERROR(g_logger, "snd_pcm_hw_params_get_rate_max returned error=" << rc << " " << snd_strerror(rc));
                return;
            }

            // set the sample rate
            rc = snd_pcm_hw_params_set_rate(device_handle_p, hw_params_p, rate, 0);
            if (rc < 0)
            {
                LOG4CXX_ERROR(g_logger, "snd_pcm_hw_params_set_rate_near returned error=" << rc << " " << snd_strerror(rc));
                return;
            }

            // specify that we want non-interleaved data
            rc = snd_pcm_hw_params_set_access(device_handle_p, hw_params_p, SND_PCM_ACCESS_RW_INTERLEAVED);
            if (rc < 0)
            {
                LOG4CXX_ERROR(g_logger, "snd_pcm_hw_params_set_access returned error=" << rc << " " << snd_strerror(rc));
                return;
            }

            // try to set the hardware params
            // if we can't then this is a buggy device that we want to ignore
            rc = snd_pcm_hw_params(device_handle_p, hw_params_p);
            if (0 > rc)
            {
                LOG4CXX_WARN(g_logger, "buggy device=" << device << " detected, ignoring");
                // close the device
                snd_pcm_close(device_handle_p);
                // release the memory
                snd_pcm_hw_params_free(hw_params_p);
                // go to the next device
                continue;
            }

            // allocate the capture instance
            AUDIOCaptureInstance *instance_p = new AUDIOCaptureInstance(this, device, channel_count, format, rate, device_handle_p);
            // store it 
            m_instances.push_back(instance_p);
            // release the memory
            snd_pcm_hw_params_free(hw_params_p);            
        }
        // release the handler
        snd_ctl_close(card_handle_p);
    }

    LOG4CXX_TRACE(g_logger, "AUDIOCaptureManager:AUDIOCaptureManager exit");
}

AUDIOChannel::Index AUDIOCaptureManager::allocate_index()
{
    LOG4CXX_TRACE(g_logger, "AUDIOCaptureManager::allocate_index enter");

    m_channel_count++;
    AUDIOChannel::Index index = (AUDIOChannel::Index)m_channel_count;

    LOG4CXX_TRACE(g_logger, "AUDIOCaptureManager::allocate_index exit " << index);
    return index;
}


