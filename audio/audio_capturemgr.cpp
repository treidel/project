
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

#define CAPTURE_DEVICE "hw:1,0"

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

    AUDIOChannel *channel_p = m_channels_map[index];

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

    // TBD: dynamically discover the available audio capture devices

    // allocate the array for the capture instances
    AUDIOCaptureInstance *instance_p = new AUDIOCaptureInstance(this, CAPTURE_DEVICE);

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


