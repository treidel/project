
#include "common.h"
#include "audio_channel.h"
#include "audio_capturemgr.h"
#include "log.h"

#include <unistd.h>
#include <stdlib.h>

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

static LogInstance g_logger("audio.channel");

///////////////////////////////////////////////////////////////////////////////
// private function declarations
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// public function implementations
///////////////////////////////////////////////////////////////////////////////

AUDIOChannel::AUDIOChannel(Index index, unsigned int sample_rate, float fullscale_voltage) :
    m_index(index),
    m_fullscale_voltage(fullscale_voltage),
    m_sample_rate(sample_rate),
    m_loop_p(ev_default_loop(0))
{
    LOG_GENERATE_TRACE(g_logger, "AUDIOChannel::AUDIOChannel enter this=%p index=%d sample_rate=%d fullscale_voltage=%f", this, index, sample_rate, fullscale_voltage);

    // allocate the pipe file descriptors
    int fds[2];
    int rc = pipe(fds);
    if (0 > rc)
    {
        LOG_GENERATE_ERROR(g_logger, "pipe returned error=%d", rc);
        return;
    }
    // store the file descriptors in our private data
    m_read_fd = fds[0];
    m_write_fd = fds[1];

    // create the watcher
    ev_io_init(&m_watcher, read_cb, m_read_fd, EV_READ);
    m_watcher.data = (void *)this;
    // register the listener with the loop
    ev_io_start(m_loop_p, &m_watcher);

    LOG_GENERATE_TRACE(g_logger, "AUDIOChannel::AUDIOChannel exit");
}

AUDIOChannel::~AUDIOChannel()
{
    LOG_GENERATE_TRACE(g_logger, "AUDIOChannel::~AUDIOChannel enter this=%p", this);

    // stop the watcher
    ev_io_stop(m_loop_p, &m_watcher);
    // close the file descriptors for the pipe
    close(m_read_fd);
    close(m_write_fd);

    LOG_GENERATE_TRACE(g_logger, "AUDIOChannel::AUDIOChannel exit");
}


///////////////////////////////////////////////////////////////////////////////
// private function implementations
///////////////////////////////////////////////////////////////////////////////

void AUDIOChannel::read_cb(struct ev_loop *loop_p, struct ev_io *w_p, int revents)
{
    LOG_GENERATE_TRACE(g_logger, "AUDIOChannel::read_cb enter loop_p=%p w_p=%p revents=0x%x", loop_p, w_p, revents);

    // get our object
    AUDIOChannel *channel_p = (AUDIOChannel *)w_p->data;

    // calculate the number of samples in 50ms of samples 
    const unsigned int num_samples = CALC_NUM_SAMPLES_FOR_MILLIS(50, channel_p->get_sample_rate());

    // setup buffer space on the stack for the audio samples
    AUDIOChannel::Sample buffer[num_samples];

    // read the data from the pipe
    int rc = read(channel_p->get_read_fd(), buffer, sizeof(buffer));
    if (0 > rc)
    {
        LOG_GENERATE_ERROR(g_logger, "read returned error=%d", rc);
        return;
    }
    // iterate through all handlers
    AUDIOCaptureManager *manager_p = AUDIOCaptureManager::get_instance();
    for (std::list<AUDIOCaptureManager::Handler *>::iterator iter=manager_p->m_handlers.begin();
            iter != manager_p->m_handlers.end();
            ++iter)
    {
        // call the handler to do something useful with this audio frame
        AUDIOCaptureManager::Handler *handler_p = *iter;
        ResultCode result = handler_p->handle_samples(channel_p, num_samples, buffer);
        if (RESULT_CODE_OK != result)
        {
            LOG_GENERATE_ERROR(g_logger, "handler_p->handle_samples returned error=%d", result);
            return;
        }
    }

    LOG_GENERATE_TRACE(g_logger, "AUDIOChannel::read_cb exit");
}


