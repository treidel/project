
// this line is required to get the limit macros
#define __STDC_LIMIT_MACROS

#include "audio_processor.h"
#include "audio_capturemgr.h"

#include <ev.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdint.h>

#include <log4cxx/logger.h>

///////////////////////////////////////////////////////////////////////////////
// macros
///////////////////////////////////////////////////////////////////////////////

#define UPDATES_PER_SECOND (10)
#define UPDATE_FREQUENCY (1.0f/UPDATES_PER_SECOND)
#define VU_NUMBER_OF_SAMPLES(x)	   ((3 * x) / 10)   // 300ms of samples
#define FULL_SCALE_VOLTAGE  (3.3f)
#define ZERO_DB_RMS_VOLTAGE (0.775f)
#define ZERO_DB_PEAK_VOLTAGE (1.0f)
#define ZERO_VU_LEVEL_IN_DB (4.0f)

///////////////////////////////////////////////////////////////////////////////
// type defintions
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// constants
///////////////////////////////////////////////////////////////////////////////

static const float c_zero_level_in_db = -96;
static const float c_zero_level_in_vu = -20;

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
    m_loop_p(ev_default_loop(0))
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

void AUDIOProcessor::set_level_type_for_channel(LevelType level_type, AUDIOChannel *channel_p)
{
    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::set_level_type_for_channel enter " + to_string(level_type) + " " << channel_p);

    // release the meter (if t exist)
    std::map<AUDIOChannel::Index, Meter *>::iterator it = m_meters_map.find(channel_p->get_index());
    if (it != m_meters_map.end())
    {
        Meter *meter_p = it->second;
        // delete the meter
        delete meter_p;
        // erase it from the lookup
        m_meters_map.erase(it);
    }

    // depending on the type we need to setup the meter
    switch(level_type)
    {
    case LEVEL_TYPE_NONE:
        // nothing to do
        break;

    case LEVEL_TYPE_PPM:
    {
        // create the peak meter
        Meter *meter_p = new PPMMeter(channel_p);
        // store it
        m_meters_map[channel_p->get_index()] = meter_p;
    }
    break;

    case LEVEL_TYPE_DIGITALPEAK:
    {
        // create the peak meter
        Meter *meter_p = new DigitalPeakMeter(channel_p);
        // store it
        m_meters_map[channel_p->get_index()] = meter_p;
    }
    break;

    case LEVEL_TYPE_VU:
    {
        // create the peak meter
        Meter *meter_p = new VUMeter(channel_p);
        // store it
        m_meters_map[channel_p->get_index()] = meter_p;
    }
    break;

    default:
        LOG4CXX_ERROR(g_logger, "unknown level type=" << level_type);
        return;
    }

    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::set_level_type_for_channel exit");
}

AUDIOProcessor::LevelType AUDIOProcessor::get_level_type_for_channel(AUDIOChannel *channel_p) const
{
    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::set_level_type_for_channel enter " << channel_p);

    // if there is no meter then this is the type
    LevelType type = LEVEL_TYPE_NONE;

    // find the meter (if it exists)
    Meter *meter_p = find_meter_by_channel_index(channel_p->get_index());
    if (NULL != meter_p)
    {
        type = meter_p->get_level_type();
    }

    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::get_level_type_for_channel exit " << type);
    return type;
}

ResultCode AUDIOProcessor::handle_samples(AUDIOChannel *channel_p, const size_t buffer_length, AUDIOChannel::Sample *buffer_p)
{
    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::handler_samples enter " << channel_p << " " << buffer_length << " " << buffer_p);

    // if we have a meter let it do its thing
    Meter *meter_p = find_meter_by_channel_index(channel_p->get_index());
    if (NULL != meter_p)
    {
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

AUDIOProcessor::PPMMeter::PPMMeter(AUDIOChannel *channel_p) :
    AUDIOProcessor::Meter(channel_p),
    m_peak(AUDIO_CHANNEL_ZERO_LEVEL)
{
    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::PPMMeter::PPMMeter enter");
    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::PPMMeter::PPMMeter exit");
}

AUDIOProcessor::PPMMeter::~PPMMeter()
{
    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::PPMMeter::~PPMMeter enter");
    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::PPMMeter::~PPMMeter exit");
}

void AUDIOProcessor::PPMMeter::process_samples(const size_t buffer_length, AUDIOChannel::Sample *buffer_p)
{
    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::PPMMeter::process_samples enter " + to_string(buffer_length) + " " + to_string(buffer_p));

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

    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::PPMMeter::process_samples exit");
}

AUDIOProcessor::ResultData AUDIOProcessor::PPMMeter::create_result_data()
{
    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::PPMMeter::create_result_data enter");

    AUDIOProcessor::ResultData data;
    memset(&data, 0, sizeof(data));

    // populate the type
    data.type = LEVEL_TYPE_PPM;

    // populate the channel
    data.channel = get_channel_p()->get_index();

    // assume the level is zero for now
    data.values.peakInDB = c_zero_level_in_db;
    // if this is zero then the level is dB is negative infinity
    if (AUDIO_CHANNEL_ZERO_LEVEL != m_peak)
    {
        // convert the normalized sample to a voltage
        float voltage = m_peak * FULL_SCALE_VOLTAGE;    
        // do the voltage to dBu conversion 
        data.values.peakInDB = 20.f * log10f(voltage / ZERO_DB_PEAK_VOLTAGE);
    }

    // reset the peak
    m_peak = AUDIO_CHANNEL_ZERO_LEVEL;

    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::PPMMeter::create_result_data exit");
    return data;
}

AUDIOProcessor::DigitalPeakMeter::DigitalPeakMeter(AUDIOChannel *channel_p) :
    AUDIOProcessor::Meter(channel_p),
    m_peak(AUDIO_CHANNEL_ZERO_LEVEL)
{
    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::DigitalPeakMeter::DigitalPeakMeter enter");
    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::DigitalPeakMeter::DigitalPeakMeter exit");
}

AUDIOProcessor::DigitalPeakMeter::~DigitalPeakMeter()
{
    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::DigitalPeakMeter::~DigitalPeakMeter enter");
    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::DigitalPeakMeter::~DigitalPeakMeter exit");
}

void AUDIOProcessor::DigitalPeakMeter::process_samples(const size_t buffer_length, AUDIOChannel::Sample *buffer_p)
{
    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::DigitalPeakMeter::process_samples enter " + to_string(buffer_length) + " " + to_string(buffer_p));

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

    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::DigitalPeakMeter::process_samples exit");
}

AUDIOProcessor::ResultData AUDIOProcessor::DigitalPeakMeter::create_result_data()
{
    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::DigitalPeakMeter::create_result_data enter");

    AUDIOProcessor::ResultData data;
    memset(&data, 0, sizeof(data));

    // populate the type
    data.type = LEVEL_TYPE_DIGITALPEAK;

    // populate the channel
    data.channel = get_channel_p()->get_index();

    // assume the level is zero for now
    data.values.peakInDB = c_zero_level_in_db;
    // if this is zero then the level is dB is negative infinity
    if (AUDIO_CHANNEL_ZERO_LEVEL != m_peak)
    {
        // do the voltage to dB conversion 
        data.values.peakInDB = 20.f * log10f(m_peak);
    }

    // reset the peak
    m_peak = AUDIO_CHANNEL_ZERO_LEVEL;

    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::DigitalPeakMeter::create_result_data exit");
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
    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::VUMeter::process_samples enter " + to_string(buffer_length) + " " + to_string(buffer_p));
    // store the samples we've received
    for (int counter = 0; counter < buffer_length; counter++)
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

    // populate the type
    data.type = LEVEL_TYPE_VU;

    // populate the channel
    data.channel = get_channel_p()->get_index();

    // assume for now that there is no signal
    float voltageInDB = c_zero_level_in_db;

    // calculate the root-mean-squared (RMS) value of the 300ms of audio we have cached
    float sum_squares = 0.0f;
    for (int counter = 0; counter < m_sample_count; counter++)
    {
        // convert the normalized value into a voltage 
        float voltage = m_samples_p[counter] * FULL_SCALE_VOLTAGE;
        // square the voltage
        sum_squares += voltage * voltage;
    }
    // round up to zero just in case some float weirdness resulted in a negative number
    sum_squares = std::max(sum_squares, 0.0f);

    // only convert to DB if there was a signal
    if (0.0f < sum_squares)
    {
        // finish the RMS calculation
        float rms = sqrtf(sum_squares / m_sample_count);
        voltageInDB = 20.0f * log10f(rms / ZERO_DB_RMS_VOLTAGE);
    }

    // convert dBm to VU where 4 dBM = 0VU
    data.values.vuInUnits = voltageInDB - ZERO_VU_LEVEL_IN_DB;

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

    // we only call the handler if we have a handler
    if (NULL != processor_p->m_handler_p)
    {
        // size the result array to the number of meters we have
        const size_t channel_count = processor_p->m_meters_map.size();
        // see if we have any active channels
        if (0 < channel_count)
        {
            ResultData result_data[channel_count];
        
            // iterate through all meters
            size_t index = 0;
            for (std::map<AUDIOChannel::Index, Meter *>::iterator it = processor_p->m_meters_map.begin();
                it != processor_p->m_meters_map.end();
                it++)
            {
                // get the meter
                Meter *meter_p = it->second;
                // get the channel
                const AUDIOChannel *channel_p = meter_p->get_channel_p();
                // generate the result data
                result_data[index] = meter_p->create_result_data();
                index++;
            }

            // call the handler
            processor_p->m_handler_p->handle_results(channel_count, result_data);
        }
    }

    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::timer_cb exit");
}

AUDIOProcessor::Meter *AUDIOProcessor::find_meter_by_channel_index(AUDIOChannel::Index index) const
{
    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::find_meter_by_channel_index enter " + to_string(index));

    std::map<AUDIOChannel::Index, Meter *>::const_iterator it = m_meters_map.find(index);
    // assume we won't find it 
    Meter *meter_p = NULL;
    if (it != m_meters_map.end())
    {
        meter_p = it->second;
    }
    // done
    LOG4CXX_DEBUG(g_logger, "AUDIOProcessor::find_meter_by_channel_index exit " << meter_p);
    return meter_p;
}
