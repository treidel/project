
#include "common.h"
#include "audio_channel.h"
#include "audio_capturemgr.h"

#include <unistd.h>
#include <stdlib.h>

///////////////////////////////////////////////////////////////////////////////
// macros
///////////////////////////////////////////////////////////////////////////////

#define BUFFER_SIZE_IN_SAMPLES (128)
#define BUFFER_SIZE_IN_BYTES (BUFFER_SIZE_IN_SAMPLES * sizeof(AUDIOChannel::Sample))

///////////////////////////////////////////////////////////////////////////////
// type defintions
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// constants
///////////////////////////////////////////////////////////////////////////////

static const AUDIOChannel::NormalizedSample c_normalization_factor = (1.0 / 32768.0);

///////////////////////////////////////////////////////////////////////////////
// module variables
///////////////////////////////////////////////////////////////////////////////


// we get away with having a single buffer by virtue of being single threaded
// in event loop
static AUDIOChannel::Sample *g_buffer_p = (AUDIOChannel::Sample *)malloc(BUFFER_SIZE_IN_BYTES);


///////////////////////////////////////////////////////////////////////////////
// private function declarations
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// public function implementations
///////////////////////////////////////////////////////////////////////////////


AUDIOChannel::NormalizedSample AUDIOChannel::convertToNormalized(AUDIOChannel::Sample sample)
{
	return sample * c_normalization_factor;
}

AUDIOChannel::AUDIOChannel(Index index) :
	m_index(index),
	m_loop_p(ev_default_loop(0))
{
        // allocate the pipe file descriptors
        int fds[2];
        if (0 > pipe(fds))
        {
            	// TBD: error log 
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
}

AUDIOChannel::~AUDIOChannel() 
{
	// stop the watcher
	ev_io_stop(m_loop_p, &m_watcher);
	// close the file descriptors for the pipe
	close(m_read_fd);
	close(m_write_fd);
}

///////////////////////////////////////////////////////////////////////////////
// private function implementations
///////////////////////////////////////////////////////////////////////////////

void AUDIOChannel::read_cb(struct ev_loop *loop_p, struct ev_io *w_p, int revents)
{
	// get our object
	AUDIOChannel *channel_p = (AUDIOChannel *)w_p->data;
	// read the data from the pipe
	if (0 > read(channel_p->getReadFD(), g_buffer_p, BUFFER_SIZE_IN_BYTES))
	{
		// TBD: error
		return;
	}
	// iterate through all handlers
	AUDIOCaptureManager *manager_p = AUDIOCaptureManager::getInstance();
	for (std::list<AUDIOCaptureManager::Handler *>::iterator iter=manager_p->m_handlers.begin(); 
             iter != manager_p->m_handlers.end(); 
             ++iter)
	{
		// call the handler to do something useful with this audio frame
		AUDIOCaptureManager::Handler *handler_p = *iter;
		if (RESULT_CODE_OK != handler_p->handle_samples(channel_p->getIndex(), BUFFER_SIZE_IN_SAMPLES, g_buffer_p))
		{
			// TBD: error
			return;
		}
	}
}


