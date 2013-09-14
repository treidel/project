
// this line is required to get the limit macros
#define __STDC_LIMIT_MACROS

#include "audio_processor.h"
#include "audio_capturemgr.h"

#include <ev.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#include <log4cxx/logger.h>

///////////////////////////////////////////////////////////////////////////////
// macros
///////////////////////////////////////////////////////////////////////////////

#define UPDATES_PER_SECOND      (24)
#define UPDATE_FREQUENCY        (1.0f/UPDATES_PER_SECOND)
#define VU_NUMBER_OF_SAMPLES(x)	((3 * x) / 10)   // 300ms of samples
#define FULL_SCALE_VOLTAGE      (3.3f)
#define ZERO_DB_RMS_VOLTAGE     (0.775f)
#define ZERO_DB_PEAK_VOLTAGE    (1.0f)
#define ZERO_VU_LEVEL_IN_DB     (4.0f)

// reference: http://en.wikipedia.org/wiki/Peak_programme_meter
// -2dB (80%) in 5ms -> 40% in 2.5ms
#define PPM_TYPE_1_INTEGRATION_RISE_LEVEL (0.4f)
#define PPM_TYPE_1_INTEGRATION_RISE_TIME  (0.0025f)
// -24dB in 1.7s 
#define PPM_TYPE_1_INTEGRATION_DROP_LEVEL  (powf(10.0f, (-24.0f / 20.f)))
#define PPM_TYPE_1_INTEGRATION_DROP_TIME   (2.8f)

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
    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::AUDIOProcessor enter");

    // register for audio data
    AUDIOCaptureManager::get_instance()->add_handler(this);

    // setup the periodic timer
    ev_timer_init(&m_timer, timer_cb, UPDATE_FREQUENCY, UPDATE_FREQUENCY);
    m_timer.data = (void *)this;
    ev_timer_start(m_loop_p, &m_timer);

    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::AUDIOProcessor exit");
}

AUDIOProcessor::~AUDIOProcessor()
{
    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::~AUDIOProcessor enter");

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

    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::~AUDIOProcessor exit");
}

void AUDIOProcessor::add_meter(Meter *meter_p)
{
    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::add_meter enter " << meter_p);

    // release the existing meter (if it exists)
    std::map<AUDIOChannel::Index, Meter *>::iterator it = m_meters_map.find(meter_p->get_channel_p()->get_index());
    if (it != m_meters_map.end())
    {
        // delete the meter
        delete it->second;
        // erase it from the lookup
        m_meters_map.erase(it);
    }

    // store the new meter
    m_meters_map[meter_p->get_channel_p()->get_index()] = meter_p;

    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::add_meter exit");
}

void AUDIOProcessor::clear_meter(AUDIOChannel *channel_p)
{
    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::clear_meter enter " << channel_p);

    // release the existing meter (if it exists)
    std::map<AUDIOChannel::Index, Meter *>::iterator it = m_meters_map.find(channel_p->get_index());
    if (it != m_meters_map.end())
    {
        // delete the meter
        delete it->second;
        // erase it from the lookup
        m_meters_map.erase(it);
    }

    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::clear_meter exit");
}

const AUDIOProcessor::Meter *AUDIOProcessor::get_meter(const AUDIOChannel *channel_p) const
{
    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::get_meter enter " << channel_p);

    // find the meter (if it exists)
    Meter *meter_p = find_meter_by_channel_index(channel_p->get_index());

    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::get_level_type_for_channel exit " << meter_p);
    return meter_p;
}

ResultCode AUDIOProcessor::handle_samples(AUDIOChannel *channel_p, const size_t buffer_length, AUDIOChannel::Sample *buffer_p)
{
    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::handler_samples enter " << channel_p << " " << buffer_length << " " << buffer_p);

    // if we have a meter let it do its thing
    Meter *meter_p = find_meter_by_channel_index(channel_p->get_index());
    if (NULL != meter_p)
    {
        meter_p->process_samples(buffer_length, buffer_p);
    }

    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::handler_samples exit");
    return RESULT_CODE_OK;
}

void AUDIOProcessor::add_handler(AUDIOProcessor::Handler *handler_p)
{
    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::add_handler enter " << handler_p);

    if (NULL == m_handler_p)
    {
        m_handler_p = handler_p;
    }
    else
    {
        LOG4CXX_ERROR(g_logger, "m_handler_p is not NULL");
    }

    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::add_handler exit");
}

void AUDIOProcessor::remove_handler(AUDIOProcessor::Handler *handler_p)
{
    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::remove_handler enter " << handler_p);

    if (handler_p == m_handler_p)
    {
        m_handler_p = NULL;
    }
    else
    {
        LOG4CXX_ERROR(g_logger, "m_handler_p is not equal to handler_p");
    }

    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::remove_handler exit");
}

AUDIOProcessor::Meter::~Meter()
{
    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::Meter::~Meter enter");
    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::Meter::~Meter exit");
}

AUDIOProcessor::PeakMeter::~PeakMeter()
{
    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::PeakMeter::~PeakMeter enter");
    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::PeakMeter::~PeakMeter exit");
}

void AUDIOProcessor::PeakMeter::set_hold_time(uint32_t hold_time)
{
    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::PeakMeter::set_hold_time enter " + to_string(hold_time));

    ASSERT((METER_HOLD_TIME_INVALID == hold_time) || 
            ((METER_HOLD_TIME_MINIMUM_IN_SECS >= hold_time) && 
             (METER_HOLD_TIME_MAXIMUM_IN_SECS <= hold_time)));

    // store the hold time 
    m_hold_time = hold_time;

    // if we're now invalid clear the current hold level
    if (METER_HOLD_TIME_INVALID == hold_time)
    {
        m_hold = AUDIO_CHANNEL_ZERO_LEVEL;
    }

    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::PeakMeter::set_hold_time exit");
}

AUDIOProcessor::PPMMeter::PPMMeter(AUDIOChannel *channel_p) :
    AUDIOProcessor::PeakMeter::PeakMeter(channel_p)
{
    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::PPMMeter::PPMMeter enter");

    // calculate the rise and fall factors
    m_rise_factor = 1.0f - powf(PPM_TYPE_1_INTEGRATION_RISE_LEVEL, 1.0f / ((float)(channel_p->get_sample_rate()) * PPM_TYPE_1_INTEGRATION_RISE_TIME));
    m_fall_factor = powf(PPM_TYPE_1_INTEGRATION_DROP_LEVEL, 1.0f / ((float)(channel_p->get_sample_rate()) * PPM_TYPE_1_INTEGRATION_DROP_TIME));

    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::PPMMeter::PPMMeter exit");
}

AUDIOProcessor::PPMMeter::~PPMMeter()
{
    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::PPMMeter::~PPMMeter enter");
    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::PPMMeter::~PPMMeter exit");
}

void AUDIOProcessor::PPMMeter::process_samples(const size_t buffer_length, AUDIOChannel::Sample *buffer_p)
{
    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::PPMMeter::process_samples enter " + to_string(buffer_length) + " " + to_string(buffer_p));

    AUDIOChannel::Sample min = 1.0f;
    AUDIOChannel::Sample max = AUDIO_CHANNEL_ZERO_LEVEL;

    // calculate the peak amplitude for the signal
    for (int counter = 0; counter < buffer_length; counter++)
    {
        // take the absolute value
        AUDIOChannel::Sample amplitude = fabs(buffer_p[counter]);

        // calculate the min + max amplitude
        min = std::min(amplitude, min);
        max = std::max(amplitude, max);

        // see if this sample is larger then the current peak
        if (amplitude > get_peak())
        {
            // calculate the new peak
            float new_peak = get_peak() + m_rise_factor * (amplitude - get_peak());
            // set it 
            set_peak(new_peak);
        }
        else
        {
            // attenuate
            float new_peak = get_peak() * m_fall_factor; 
            set_peak(new_peak);
        }
    }

    LOG4CXX_DEBUG(g_logger, "ppm min=" + to_string(min) + " max=" + to_string(max));

    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::PPMMeter::process_samples exit");
}

AUDIOProcessor::ResultData AUDIOProcessor::PPMMeter::create_result_data()
{
    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::PPMMeter::create_result_data enter");

    AUDIOProcessor::ResultData data;
    memset(&data, 0, sizeof(data));

    // populate the type
    data.type = LEVEL_TYPE_PPM;

    // populate the channel
    data.channel = get_channel_p()->get_index();

    // assume the levels are zero for now
    data.values.peak.peakInDB = c_zero_level_in_db;
    data.values.peak.holdInDB = c_zero_level_in_db;
    // if this is zero then the level is dB is negative infinity
    if (AUDIO_CHANNEL_ZERO_LEVEL != get_peak())
    {
        // convert the normalized sample to a voltage
        float voltage = get_peak() * FULL_SCALE_VOLTAGE;    
        // do the voltage to dBu conversion 
        data.values.peak.peakInDB = 20.f * log10f(voltage / ZERO_DB_PEAK_VOLTAGE);
    }
    if (AUDIO_CHANNEL_ZERO_LEVEL != get_hold())
    {
        // convert the normalized sample to a voltage
        float voltage = get_hold() * FULL_SCALE_VOLTAGE;    
        // do the voltage to dBu conversion 
        data.values.peak.holdInDB = 20.f * log10f(voltage / ZERO_DB_PEAK_VOLTAGE);
    }

    LOG4CXX_DEBUG(g_logger, "ppm peak(dB)=" + to_string(data.values.peak.peakInDB) + " hold(dB)=" + to_string(data.values.peak.holdInDB));


    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::PPMMeter::create_result_data exit");
    return data;
}

AUDIOProcessor::DigitalPeakMeter::DigitalPeakMeter(AUDIOChannel *channel_p) :
    AUDIOProcessor::PeakMeter(channel_p)
{
    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::DigitalPeakMeter::DigitalPeakMeter enter");
    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::DigitalPeakMeter::DigitalPeakMeter exit");
}

AUDIOProcessor::DigitalPeakMeter::~DigitalPeakMeter()
{
    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::DigitalPeakMeter::~DigitalPeakMeter enter");
    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::DigitalPeakMeter::~DigitalPeakMeter exit");
}

void AUDIOProcessor::DigitalPeakMeter::process_samples(const size_t buffer_length, AUDIOChannel::Sample *buffer_p)
{
    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::DigitalPeakMeter::process_samples enter " + to_string(buffer_length) + " " + to_string(buffer_p));

    // calculate the peak amplitude for the signal
    for (int counter = 0; counter < buffer_length; counter++)
    {
        // take the absolute value
        AUDIOChannel::Sample amplitude = fabs(buffer_p[counter]);
        // if this sample is larger then last one calculated for the channel then
        // store it
        if (amplitude > get_peak())
        {
            set_peak(amplitude);
        }
    }

    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::DigitalPeakMeter::process_samples exit");
}

AUDIOProcessor::ResultData AUDIOProcessor::DigitalPeakMeter::create_result_data()
{
    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::DigitalPeakMeter::create_result_data enter");

    AUDIOProcessor::ResultData data;
    memset(&data, 0, sizeof(data));

    // populate the type
    data.type = LEVEL_TYPE_DIGITALPEAK;

    // populate the channel
    data.channel = get_channel_p()->get_index();

    // assume the levels are zero for now
    data.values.peak.peakInDB = c_zero_level_in_db;
    data.values.peak.holdInDB = c_zero_level_in_db;
    // if this is zero then the level is dB is negative infinity
    if (AUDIO_CHANNEL_ZERO_LEVEL != get_peak())
    {
        // do the voltage to dB conversion 
        data.values.peak.peakInDB = 20.f * log10f(get_peak());
    }
    if (AUDIO_CHANNEL_ZERO_LEVEL != get_hold())
    {
        // do the voltage to dB conversion 
        data.values.peak.holdInDB = 20.f * log10f(get_peak());
    }

    LOG4CXX_DEBUG(g_logger, "digital peak(dB)=" + to_string(data.values.peak.peakInDB) + " hold(dB)=" + to_string(data.values.peak.holdInDB));

    // reset the peak
    set_peak(AUDIO_CHANNEL_ZERO_LEVEL);

    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::DigitalPeakMeter::create_result_data exit");
    return data;
}

AUDIOProcessor::VUMeter::VUMeter(AUDIOChannel *channel_p) :
    AUDIOProcessor::Meter(channel_p),
    m_sample_index(0),
    m_sample_count(VU_NUMBER_OF_SAMPLES(channel_p->get_sample_rate()))
{
    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::VUMeter::VUMeter enter");
    m_samples_p = (AUDIOChannel::Sample *)calloc(1, sizeof(AUDIOChannel::Sample) * m_sample_count);
    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::VUMeter::VUMeter exit");
}

AUDIOProcessor::VUMeter::~VUMeter()
{
    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::VUMeter::~VUMeter enter");
    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::VUMeter::~VUMeter exit");
}

void AUDIOProcessor::VUMeter::process_samples(const size_t buffer_length, AUDIOChannel::Sample *buffer_p)
{
    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::VUMeter::process_samples enter " + to_string(buffer_length) + " " + to_string(buffer_p));
    // store the samples we've received
    for (int counter = 0; counter < buffer_length; counter++)
    {
        // copy over the sample
        m_samples_p[m_sample_index] = buffer_p[counter];
        // advance to the next sample position taking care to wrap
        m_sample_index = (m_sample_index + 1) % m_sample_count;
    }

    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::VUMeter::process_samples exit");
}

AUDIOProcessor::ResultData AUDIOProcessor::VUMeter::create_result_data()
{
    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::VUMeter::create_result_data enter");

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

    LOG4CXX_DEBUG(g_logger, "vu value(units)=" + to_string(data.values.vuInUnits));

    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::VUMeter::create_result_data exit");
    return data;
}

///////////////////////////////////////////////////////////////////////////////
// protected function implementations
///////////////////////////////////////////////////////////////////////////////

AUDIOProcessor::Meter::Meter(AUDIOChannel *channel_p) :
    m_channel_p(channel_p)
{
    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::Meter::Meter enter");
    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::Meter::Meter exit");
}


AUDIOProcessor::PeakMeter::PeakMeter(AUDIOChannel *channel_p) :
    Meter(channel_p),
    m_hold_time(METER_HOLD_TIME_INVALID),
    m_peak(AUDIO_CHANNEL_ZERO_LEVEL),
    m_hold(AUDIO_CHANNEL_ZERO_LEVEL),
    m_last_timestamp(0)
{
    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::PeakMeter::PeakMeter enter");
    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::PeakMeter::PeakMeter exit");
}

void AUDIOProcessor::PeakMeter::set_peak(AUDIOChannel::Sample peak)
{
    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::PeakMeter::set_peak enter");
    // update the peak value
    m_peak = peak;
    // see if a hold time is set
    if (METER_HOLD_TIME_INVALID != m_hold_time)
    {
        // get the current time
        time_t current_time = time(NULL); 
        // if the time is older than the hold time or if the current peak is greater 
        // then update the peak
        if (((m_last_timestamp + m_hold_time) < current_time) || 
            (peak > m_hold))
        {
            LOG4CXX_TRACE(g_logger, "updating hold value for channel=" + get_channel_p()->get_index());
            m_hold = peak;
        }
    }

    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::PeakMeter::setpeak exit");
}


///////////////////////////////////////////////////////////////////////////////
// private function implementations
///////////////////////////////////////////////////////////////////////////////

void AUDIOProcessor::timer_cb(EV_P_ ev_timer *w_p, int revents)
{
    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::timer_cb enter " << w_p << " " << revents);

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

    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::timer_cb exit");
}

AUDIOProcessor::Meter *AUDIOProcessor::find_meter_by_channel_index(AUDIOChannel::Index index) const
{
    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::find_meter_by_channel_index enter " + to_string(index));

    std::map<AUDIOChannel::Index, Meter *>::const_iterator it = m_meters_map.find(index);
    // assume we won't find it 
    Meter *meter_p = NULL;
    if (it != m_meters_map.end())
    {
        meter_p = it->second;
    }
    // done
    LOG4CXX_TRACE(g_logger, "AUDIOProcessor::find_meter_by_channel_index exit " << meter_p);
    return meter_p;
}
