
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

        snd_ctl_t *handle_p = NULL;
        if (0 > snd_ctl_open(&handle_p, card, 0))
        {
            LOG4CXX_ERROR(g_logger, "error opening card=" + to_string(card_index));
            return;
        }
        // allocate the memory for the card info
        snd_ctl_card_info_t *card_info_p = NULL;
        snd_ctl_card_info_alloca(&card_info_p);
        // query the card info
        if (0 > snd_ctl_card_info(handle_p, card_info_p))
        {
            LOG4CXX_ERROR(g_logger, "error querying card=" + to_string(card_index));
            return;
        }
        // iterate for all devices
        int device_index = -1;
        while (0 <= snd_ctl_pcm_next_device(handle_p, &device_index))
        {
            char device[3 + 10 + 1 + 10 + 1];
            sprintf(device, "hw:%d,%d", card_index, device_index);
            // query the device info asking for capture devices only
            snd_pcm_info_t *device_info_p = NULL;
            snd_pcm_info_alloca(&device_info_p);
            snd_pcm_info_set_device(device_info_p, device_index);
            snd_pcm_info_set_subdevice(device_info_p, 0);
            snd_pcm_info_set_stream(device_info_p, SND_PCM_STREAM_CAPTURE);
            int rc = snd_ctl_pcm_info(handle_p, device_info_p);
            if ((0 > rc) && (-ENOENT != rc))
            {
                LOG4CXX_ERROR(g_logger, "error querying device info card=" + to_string(card_index) + " device=" + to_string(device_index));
                return;
            }
            // allocate the capture instance
            AUDIOCaptureInstance *instance_p = new AUDIOCaptureInstance(this, device);
            // store it 
            m_instances.push_back(instance_p);
            // release the memory
            snd_pcm_info_free(device_info_p);
        }
        // release the memory
        snd_ctl_card_info_free(card_info_p);
        // release the handler
        snd_ctl_close(handle_p);
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


