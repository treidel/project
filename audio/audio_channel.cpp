
#include "common.h"
#include "audio_channel.h"
#include "audio_capturemgr.h"

#include <unistd.h>
#include <stdlib.h>

#include <log4cxx/logger.h>

///////////////////////////////////////////////////////////////////////////////
// macros
///////////////////////////////////////////////////////////////////////////////

#define BUFFER_SIZE_IN_SAMPLES (2048)
#define BUFFER_SIZE_IN_BYTES (BUFFER_SIZE_IN_SAMPLES * sizeof(AUDIOChannel::Sample))

///////////////////////////////////////////////////////////////////////////////
// type defintions
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// constants
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// module variables
///////////////////////////////////////////////////////////////////////////////

static log4cxx::LoggerPtr g_logger(log4cxx::Logger::getLogger("audio.channel"));

// we get away with having a single buffer by virtue of being single threaded
// in event loop
static AUDIOChannel::Sample g_buffer[BUFFER_SIZE_IN_BYTES];


///////////////////////////////////////////////////////////////////////////////
// private function declarations
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// public function implementations
///////////////////////////////////////////////////////////////////////////////

AUDIOChannel::AUDIOChannel(Index index) :
	m_index(index),
	m_loop_p(ev_default_loop(0))
{
	LOG4CXX_DEBUG(g_logger, "AUDIOChannel::AUDIOChannel enter " << index);

        // allocate the pipe file descriptors
        int fds[2];
        int rc = pipe(fds);
	if (0 > rc)
        {
		LOG4CXX_ERROR(g_logger, "pipe returned error " << rc);
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

	LOG4CXX_DEBUG(g_logger, "AUDIOChannel::AUDIOChannel exit");
}

AUDIOChannel::~AUDIOChannel() 
{
	LOG4CXX_DEBUG(g_logger, "AUDIOChannel::~AUDIOChannel enter");

	// stop the watcher
	ev_io_stop(m_loop_p, &m_watcher);
	// close the file descriptors for the pipe
	close(m_read_fd);
	close(m_write_fd);

	LOG4CXX_DEBUG(g_logger, "AUDIOChannel::AUDIOChannel exit");
}

///////////////////////////////////////////////////////////////////////////////
// private function implementations
///////////////////////////////////////////////////////////////////////////////

void AUDIOChannel::read_cb(struct ev_loop *loop_p, struct ev_io *w_p, int revents)
{
	LOG4CXX_DEBUG(g_logger, "AUDIOChannel::read_cb enter " << loop_p << " " << w_p << " " << revents);

	// get our object
	AUDIOChannel *channel_p = (AUDIOChannel *)w_p->data;
	// read the data from the pipe
	int rc = read(channel_p->getReadFD(), g_buffer, BUFFER_SIZE_IN_BYTES);
	if (0 > rc)
	{
		LOG4CXX_ERROR(g_logger, "read returned error " << rc);
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
		ResultCode result = handler_p->handle_samples(channel_p->getIndex(), BUFFER_SIZE_IN_SAMPLES, g_buffer);
		if (RESULT_CODE_OK != result)
		{
			LOG4CXX_ERROR(g_logger, "handler_p->handle_samples returned error " << result);
			return;
		}
	}

	LOG4CXX_DEBUG(g_logger, "AUDIOChannel::read_cb exit");
}


