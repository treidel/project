
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
    LOG4CXX_DEBUG(g_logger, "AUDIOCaptureManager::get_instance enter");

    if (NULL == g_instance_p)
    {
        g_instance_p = new AUDIOCaptureManager();
    }

    LOG4CXX_DEBUG(g_logger, "AUDIOCaptureManager::get_instance exit " << g_instance_p);

    return g_instance_p;
}

AUDIOCaptureManager::~AUDIOCaptureManager()
{
    LOG4CXX_DEBUG(g_logger, "AUDIOCaptureManager::~AUDIOCaptureManager enter");

    for (std::list<AUDIOCaptureInstance *>::iterator it = m_instances.begin();
            it != m_instances.end();
            it++)
    {
        delete *it;
    }
    g_instance_p = NULL;

    LOG4CXX_DEBUG(g_logger, "AUDIOCaptureManager::~AUDIOCaptureManager enter");
}

void AUDIOCaptureManager::add_handler(Handler *handler_p)
{
    LOG4CXX_DEBUG(g_logger, "AUDIOCaptureManager::add_handler enter " << handler_p);

    m_handlers.push_front(handler_p);

    LOG4CXX_DEBUG(g_logger, "AUDIOCaptureManager::add_handler exit");
}

void AUDIOCaptureManager::remove_handler(Handler *handler_p)
{
    LOG4CXX_DEBUG(g_logger, "AUDIOCaptureManager::remove_handler enter " << handler_p);

    m_handlers.remove(handler_p);

    LOG4CXX_DEBUG(g_logger, "AUDIOCaptureManager::remove_handler exit");
}

AUDIOChannel *AUDIOCaptureManager::find_channel(const AUDIOChannel::Index index)
{
    LOG4CXX_DEBUG(g_logger, "AUDIOCaptureManager::find_channel enter " << index);
    // assume we won't find it 
    AUDIOChannel *channel_p = NULL;
    // if we have a meter let it do its thing
    std::map<AUDIOChannel::Index, AUDIOChannel *>::iterator it = m_channels_map.find(index);
    if (m_channels_map.end() != it)
    {
        channel_p = it->second;
    }

    LOG4CXX_DEBUG(g_logger, "AUDIOCaptureManager::find_channel exit channel_p=" << channel_p);
    return channel_p;
}

void AUDIOCaptureManager::add_channel(AUDIOChannel *channel_p)
{
    LOG4CXX_DEBUG(g_logger, "AUDIOCaptureManager::add_channel enter " << channel_p);

    m_channels_map[channel_p->get_index()] = channel_p;

    LOG4CXX_DEBUG(g_logger, "AUDIOCaptureManager::add_channel exit");

}

///////////////////////////////////////////////////////////////////////////////
// private function implementations
///////////////////////////////////////////////////////////////////////////////

AUDIOCaptureManager::AUDIOCaptureManager() :
    m_channel_count(0),
    m_loop_p(ev_default_loop(0))

{
    LOG4CXX_DEBUG(g_logger, "AUDIOCaptureManager::AUDIOCaptureManager enter");

    // iterate for all cards
    int card_index = -1;
    while (0 <= snd_card_next(&card_index))
    {
        LOG4CXX_DEBUG(g_logger, "found card=" + to_string(card_index));

        char card[3 + 10 + 1];
        sprintf(card, "hw:%d", card_index);

        snd_ctl_t *card_handle_p = NULL;
        if (0 > snd_ctl_open(&card_handle_p, card, 0))
        {
            LOG4CXX_ERROR(g_logger, "error opening card=" + to_string(card_index));
            return;
        }
        // allocate the memory for the card info
        snd_ctl_card_info_t *card_info_p = NULL;
        snd_ctl_card_info_alloca(&card_info_p);
        // query the card info
        if (0 > snd_ctl_card_info(card_handle_p, card_info_p))
        {
            LOG4CXX_ERROR(g_logger, "error querying card=" + to_string(card_index));
            return;
        }
        // iterate for all devices
        int device_index = -1;
        while (0 <= snd_ctl_pcm_next_device(card_handle_p, &device_index))
        {
            char device[3 + 10 + 1 + 10 + 1];
            sprintf(device, "hw:%d,%d", card_index, device_index);
            // try to open the sound device
            snd_pcm_t *device_handle_p = NULL;
            int rc = snd_pcm_open(&device_handle_p, device, SND_PCM_STREAM_CAPTURE, SND_PCM_CLASS_GENERIC);
            if (0 > rc)
            {
                LOG4CXX_DEBUG(g_logger, "snd_pcm_open returned error=" + to_string(rc) + " " + snd_strerror(rc) + " for device=" + device);
                continue;
            }
            
            // query the hardware params
            snd_pcm_hw_params_t *hw_params_p = NULL;
            // allocate the hardware params data structure
            snd_pcm_hw_params_malloc(&hw_params_p);

            // initialize the default hardware params
            rc = snd_pcm_hw_params_any(device_handle_p, hw_params_p);
            if (rc < 0)
            {
                LOG4CXX_ERROR(g_logger, "snd_pcm_hw_params_any returned error=" << rc << " " << snd_strerror(rc));
                return;
            }

            // query the list of formats we support
            std::list<snd_pcm_format_t> format_list = AUDIOFormatterFactory::fetch_audio_format_list();

            // query the list of formats this device supports
            snd_pcm_format_mask_t *format_mask_p = NULL;
            snd_pcm_format_mask_malloc(&format_mask_p);

            // iterate for each supported list
            bool format_found = false;
            for (std::list<snd_pcm_format_t>::iterator iterator = format_list.begin();
                 iterator != format_list.end();
                 ++iterator)
            {
                if (0 != snd_pcm_format_mask_test(format_mask_p, *iterator))
                {
                    format_found = true;
                }
            }

            // release the memory
            snd_pcm_format_mask_free(format_mask_p);

            // check if we found a supported format
            if (true == format_found)
            {
                // allocate the capture instance
                AUDIOCaptureInstance *instance_p = new AUDIOCaptureInstance(this, device, device_handle_p);
                // store it 
                m_instances.push_back(instance_p);
            }
            else
            {
                LOG4CXX_DEBUG(g_logger, "no supported audio formats found for device=" << device);
                snd_pcm_close(device_handle_p);
            }
            // release the memory
            snd_pcm_hw_params_free(hw_params_p);            
        }
        // release the memory
        snd_ctl_card_info_free(card_info_p);
        // release the handler
        snd_ctl_close(card_handle_p);
    }

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


