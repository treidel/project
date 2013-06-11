
// this line is required to get the limit macros
#define __STDC_LIMIT_MACROS

#include "audio_processor.h"
#include "audio_capturemgr.h"

#include <ev.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

#include <log4cxx/logger.h>

///////////////////////////////////////////////////////////////////////////////
// macros
///////////////////////////////////////////////////////////////////////////////

#define UPDATES_PER_SECOND (10)
#define UPDATE_FREQUENCY (1.0/UPDATES_PER_SECOND)
#define VU_NUMBER_OF_SAMPLES(x)	   ((3 * x) / 10)   // 300ms of samples

///////////////////////////////////////////////////////////////////////////////
// type defintions
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// constants
///////////////////////////////////////////////////////////////////////////////

static const int32_t c_peak_zero_level_in_db = -50;
static const int32_t c_power_zero_level_in_db = -100;

///////////////////////////////////////////////////////////////////////////////
// module variables
///////////////////////////////////////////////////////////////////////////////
static log4cxx::LoggerPtr g_logger(log4cxx::Logger::getLogger("audio.processor"));

///////////////////////////////////////////////////////////////////////////////
// private function declarations
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// public function implementations
///////////////////////////////////////////////////////////////////////////////

AUDIOProcessor::AUDIOProcessor() :
    m_handler_p(NULL),
    m_loop_p(ev_default_loop(0)),
    m_level_type(LEVEL_TYPE_NONE)
{
    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::AUDIOProcessor enter");

    // register for audio data
    AUDIOCaptureManager::get_instance()->add_handler(this);

    // setup the periodic timer
    ev_timer_init(&m_timer, timer_cb, UPDATE_FREQUENCY, UPDATE_FREQUENCY);
    m_timer.data = (void *)this;
    ev_timer_start(m_loop_p, &m_timer);

    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::AUDIOProcessor exit");
}

AUDIOProcessor::~AUDIOProcessor()
{
    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::~AUDIOProcessor enter");

    // deregister
    AUDIOCaptureManager::get_instance()->remove_handler(this);

    // release the meters (if they exist)
    for (std::map<AUDIOChannel::Index, Meter *>::iterator it = m_meters_map.begin();
            it != m_meters_map.end();
            it++)
    {
        Meter *meter_p = it->second;
        delete meter_p;
    }

    // stop the timer
    ev_timer_stop(m_loop_p, &m_timer);

    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::~AUDIOProcessor exit");
}

void AUDIOProcessor::set_level_type(LevelType level_type)
{
    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::level_type enter " << level_type);

    AUDIOCaptureManager *manager_p = AUDIOCaptureManager::get_instance();

    // release the meters (if they exist)
    for(std::map<AUDIOChannel::Index, Meter *>::iterator it = m_meters_map.begin();
            it != m_meters_map.end();
            it++)
    {
        Meter *meter_p = it->second;
        delete meter_p;
    }
    m_meters_map.clear();

    // depending on the type we need to setup the meters
    switch(level_type)
    {
    case LEVEL_TYPE_NONE:
        // nothing to do
        break;

    case LEVEL_TYPE_PEAK:
    {
        for(AUDIOCaptureManager::ChannelIterator it = manager_p->begin();
                it != manager_p->end();
                it++)
        {
            // create the peak meter
            Meter *meter_p = new PeakMeter(it->second);
            // store it
            m_meters_map[it->first] = meter_p;
        }
    }
    break;

    case LEVEL_TYPE_VU:
    {
        for(AUDIOCaptureManager::ChannelIterator it = manager_p->begin();
                it != manager_p->end();
                it++)
        {
            // create the peak meter
            Meter *meter_p = new VUMeter(it->second);
            // store it
            m_meters_map[it->first] = meter_p;
        }
    }
    break;

    default:
        LOG4CXX_ERROR(g_logger, "unknown level type=" << level_type);
        return;
    }

    // store the level
    m_level_type = level_type;

    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::level_type exit");
}

ResultCode AUDIOProcessor::handle_samples(AUDIOChannel::Index index, const size_t buffer_length, AUDIOChannel::Sample *buffer_p)
{
    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::handler_samples enter " << index << " " << buffer_length << " " << buffer_p);

    // fetch the number of channels we have
    size_t channel_count = AUDIOCaptureManager::get_instance()->channel_count();

    // range check input
    ASSERT((index > 0) && (index <= channel_count));

    // if we have a meter let it do its thing
    std::map<AUDIOChannel::Index, Meter *>::iterator it = m_meters_map.find(index);
    if (m_meters_map.end() != it)
    {
        Meter *meter_p = it->second;
        meter_p->process_samples(buffer_length, buffer_p);
    }

    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::handler_samples exit");

    return RESULT_CODE_OK;
}

void AUDIOProcessor::add_handler(AUDIOProcessor::Handler *handler_p)
{
    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::add_handler enter " << handler_p);

    if (NULL == m_handler_p)
    {
        m_handler_p = handler_p;
    }
    else
    {
        LOG4CXX_ERROR(g_logger, "m_handler_p is not NULL");
    }

    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::add_handler exit");
}

void AUDIOProcessor::remove_handler(AUDIOProcessor::Handler *handler_p)
{
    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::remove_handler enter " << handler_p);

    if (handler_p == m_handler_p)
    {
        m_handler_p = NULL;
    }
    else
    {
        LOG4CXX_ERROR(g_logger, "m_handler_p is not equal to handler_p");
    }

    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::remove_handler exit");
}

AUDIOProcessor::Meter::~Meter()
{
    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::Meter::~Meter enter");
    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::Meter::~Meter exit");
}

AUDIOProcessor::PeakMeter::PeakMeter(AUDIOChannel *channel_p) :
    AUDIOProcessor::Meter(channel_p),
    m_peak(AUDIO_CHANNEL_ZERO_LEVEL)
{
    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::PeakMeter::PeakMeter enter");
    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::PeakMeter::PeakMeter exit");
}

AUDIOProcessor::PeakMeter::~PeakMeter()
{
    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::PeakMeter::~PeakMeter enter");
    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::PeakMeter::~PeakMeter exit");
}

void AUDIOProcessor::PeakMeter::process_samples(const size_t buffer_length, AUDIOChannel::Sample *buffer_p)
{
    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::PeakMeter::process_samples enter " << buffer_length << " " << buffer_p);

    // calculate the peak amplitude for the signal
    for (int counter = 0; counter < buffer_length; counter++)
    {
        // take the absolute value
        AUDIOChannel::Sample amplitude = fabs(buffer_p[counter]);
        // if this sample is larger then last one calculated for the channel then
        // store it
        if (amplitude > m_peak)
        {
            m_peak = amplitude;
        }
    }

    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::PeakMeter::process_samples exit");
}

AUDIOProcessor::ResultData AUDIOProcessor::PeakMeter::create_result_data()
{
    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::PeakMeter::create_result_data enter");

    AUDIOProcessor::ResultData data;
    memset(&data, 0, sizeof(data));

    // populate the channel
    data.channel = get_channel_p()->get_index();

    // assume the level is zero for now
    data.values.peakInDB = c_peak_zero_level_in_db;
    // if this is zero then the level is dB is negative infinity
    if (AUDIO_CHANNEL_ZERO_LEVEL != m_peak)
    {
        // do the dBm for level calcuation
        data.values.peakInDB = (int32_t)(10.0 * log10(m_peak));
    }

    // reset the peak
    m_peak = AUDIO_CHANNEL_ZERO_LEVEL;

    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::PeakMeter::create_result_data exit");
    return data;
}

AUDIOProcessor::VUMeter::VUMeter(AUDIOChannel *channel_p) :
    AUDIOProcessor::Meter(channel_p),
    m_sample_index(0),
    m_sample_count(VU_NUMBER_OF_SAMPLES(channel_p->get_sample_rate()))
{
    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::VUMeter::VUMeter enter");
    m_samples_p = (AUDIOChannel::Sample *)calloc(1, sizeof(AUDIOChannel::Sample) * m_sample_count);
    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::VUMeter::VUMeter exit");
}

AUDIOProcessor::VUMeter::~VUMeter()
{
    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::VUMeter::~VUMeter enter");
    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::VUMeter::~VUMeter exit");
}

void AUDIOProcessor::VUMeter::process_samples(const size_t buffer_length, AUDIOChannel::Sample *buffer_p)
{
    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::VUMeter::process_samples enter " << buffer_length << " " << buffer_p);
    // store the samples we've received
    for (int counter = 0; counter < m_sample_count; counter++)
    {
        // copy over the sample
        m_samples_p[m_sample_index] = buffer_p[counter];
        // advance to the next sample position taking care to wrap
        m_sample_index = (m_sample_index + 1) % m_sample_count;
    }

    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::VUMeter::process_samples exit");
}

AUDIOProcessor::ResultData AUDIOProcessor::VUMeter::create_result_data()
{
    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::VUMeter::create_result_data enter");

    AUDIOProcessor::ResultData data;
    memset(&data, 0, sizeof(data));

    // populate the channel
    data.channel = get_channel_p()->get_index();

    // assume for now that there is no signal
    data.values.powerInDB = c_power_zero_level_in_db;

    // we need to calculate the root-mean-squared (RMS) value of the 300ms of audio
    // we have cached
    float sum_squares = 0.0f;
    for (int counter = 0; counter < m_sample_count; counter++)
    {
        sum_squares += m_samples_p[counter] * m_samples_p[counter];
    }
    // round up to zero just in case some float weirdness resulted in a negative number
    sum_squares = std::max(sum_squares, 0.0f);

    // only convert to DB if there was a signal
    if (0.0f < sum_squares)
    {
        data.values.powerInDB = (int32_t)(20.0f * log10(sum_squares / m_sample_count));
    }

    // can't go lower than the minimum
    data.values.powerInDB = std::max(data.values.powerInDB, c_power_zero_level_in_db);

    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::VUMeter::create_result_data exit");
    return data;
}

///////////////////////////////////////////////////////////////////////////////
// protected function implementations
///////////////////////////////////////////////////////////////////////////////

AUDIOProcessor::Meter::Meter(AUDIOChannel *channel_p) :
    m_channel_p(channel_p)
{
    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::Meter::Meter enter");
    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::Meter::Meter exit");
}

///////////////////////////////////////////////////////////////////////////////
// private function implementations
///////////////////////////////////////////////////////////////////////////////

void AUDIOProcessor::timer_cb(EV_P_ ev_timer *w_p, int revents)
{
    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::timer_cb enter " << w_p << " " << revents);

    // get the object
    AUDIOProcessor *processor_p = (AUDIOProcessor *)w_p->data;

    // we only call the handler we are not disabled and we have a handler
    if ((NULL != processor_p->m_handler_p) && (LEVEL_TYPE_NONE != processor_p->m_level_type))
    {
        // get the manager object
        AUDIOCaptureManager *manager_p = AUDIOCaptureManager::get_instance();
        // need the channel count to size the result array
        const size_t channel_count = manager_p->channel_count();
        ResultData result_data[channel_count];
        for (AUDIOCaptureManager::ChannelIterator it = manager_p->begin();
                it != manager_p->end();
                it++)
        {
            // get the channel
            AUDIOChannel *channel_p = it->second;
            // find the meter
            Meter *meter_p = processor_p->m_meters_map[channel_p->get_index()];
            ASSERT(NULL != meter_p);
            // generate the result data
            result_data[channel_p->get_index() - 1] = meter_p->create_result_data();
        }
        // call the handler
        processor_p->m_handler_p->handle_results(channel_count, result_data);
    }

    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::timer_cb exit");
}


