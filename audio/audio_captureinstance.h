#ifndef _AUDIO_CAPTUREINSTANCE_H_
#define _AUDIO_CAPTUREINSTANCE_H_

///////////////////////////////////////////////////////////////////////////////
// includes
///////////////////////////////////////////////////////////////////////////////

#include "common.h"

#include <ev.h>
#include <alsa/asoundlib.h>
#include <pthread.h>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
// macros
///////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////
// forward declarations
///////////////////////////////////////////////////////////////////////////////

class AUDIOCaptureManager;
class AUDIOChannel;
class AUDIOFormatter;

///////////////////////////////////////////////////////////////////////////////
// class definition
///////////////////////////////////////////////////////////////////////////////

class AUDIOCaptureInstance
{
///////////////////////////////////////////////////////////////////////////////
// type definitions
///////////////////////////////////////////////////////////////////////////////

public:


///////////////////////////////////////////////////////////////////////////////
// public function declarations
///////////////////////////////////////////////////////////////////////////////

public:

    AUDIOCaptureInstance(AUDIOCaptureManager *manager_p, const char* device, size_t channel_count, snd_pcm_format_t format, unsigned int rate, snd_pcm_t *handle_p);
    virtual ~AUDIOCaptureInstance();

///////////////////////////////////////////////////////////////////////////////
// inner class declarations
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// private function declarations
///////////////////////////////////////////////////////////////////////////////

private:

    static void *thread_handler(void *arg);

///////////////////////////////////////////////////////////////////////////////
// private variable definitions
///////////////////////////////////////////////////////////////////////////////

private:
    std::vector<AUDIOChannel *> m_channels;
    AUDIOFormatter *m_formatter_p;
    snd_pcm_t *m_handle_p;
    char *m_device;
    pthread_t m_thread_id;
    size_t m_channel_count;
    bool m_abort;

};
#endif
