#ifndef _AUDIO_PROCESSOR_H_
#define _AUDIO_PROCESSOR_H_

#include "common.h"
#include "audio_channel.h"
#include "audio_capturemgr.h"

#include <ev.h>
#include <map>

///////////////////////////////////////////////////////////////////////////////
// macros
///////////////////////////////////////////////////////////////////////////////

#define METER_HOLD_TIME_INVALID            (0)
#define METER_HOLD_TIME_MINIMUM_IN_SECS    (1)
#define METER_HOLD_TIME_MAXIMUM_IN_SECS    (10)

///////////////////////////////////////////////////////////////////////////////
// forward declarations
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// class definition
///////////////////////////////////////////////////////////////////////////////

class AUDIOProcessor : public AUDIOCaptureManager::Handler
{

///////////////////////////////////////////////////////////////////////////////
// type definitions
///////////////////////////////////////////////////////////////////////////////

public:

    typedef enum
    {
        LEVEL_TYPE_NONE = 0,
        LEVEL_TYPE_DIGITALPEAK = 1,
        LEVEL_TYPE_PPM = 2,
        LEVEL_TYPE_VU = 3
    } LevelType;

    typedef struct
    {
        AUDIOChannel::Index channel;
        LevelType type;
        union
        {
            float vuInUnits;
            struct 
            {
                float peakInDB;
                float holdInDB;
            } peak;
        } values;
    } ResultData;

    class Handler
    {
    public:
        virtual ResultCode handle_results(const size_t num_results, const ResultData results[]) = 0;
    };

    class Meter
    {
    public:
        virtual ~Meter();
        virtual void process_samples(const size_t buffer_length, AUDIOChannel::Sample *buffer_p) = 0;
        virtual ResultData create_result_data() = 0;
        virtual LevelType get_level_type() = 0;
        inline const AUDIOChannel *get_channel_p() const;

    protected:
        Meter(AUDIOChannel *channel_p);
    private:
        AUDIOChannel *m_channel_p;
    };

    class PeakMeter : public Meter
    {
    public:
        virtual ~PeakMeter();
        void set_hold_time(uint32_t hold_time_in_secs);
        inline uint32_t get_hold_time() const;
    protected:
        PeakMeter(AUDIOChannel *channel_p);
        inline AUDIOChannel::Sample get_peak() const;
        void set_peak(AUDIOChannel::Sample peak);
        inline AUDIOChannel::Sample get_hold() const;
    private:
        AUDIOChannel::Sample m_peak;
        AUDIOChannel::Sample m_hold;
        uint32_t m_hold_time;
        time_t m_last_timestamp;
    };

    class PPMMeter : public PeakMeter
    {
    public:
        PPMMeter(AUDIOChannel *channel_p);
        virtual ~PPMMeter();
        void process_samples(const size_t buffer_length, AUDIOChannel::Sample *buffer_p);
        ResultData create_result_data();
        inline LevelType get_level_type();
    private:
        float m_rise_factor;
        float m_fall_factor;
    };

    class DigitalPeakMeter : public PeakMeter
    {
    public:
        DigitalPeakMeter(AUDIOChannel *channel_p);
        virtual ~DigitalPeakMeter();
        void process_samples(const size_t buffer_length, AUDIOChannel::Sample *buffer_p);
        ResultData create_result_data();
        inline LevelType get_level_type();
    private:
    };  

    class VUMeter : public Meter
    {
    public:
        VUMeter(AUDIOChannel *channel_p);
        virtual ~VUMeter();
        void process_samples(const size_t buffer_length, AUDIOChannel::Sample *buffer_p);
        ResultData create_result_data();
        inline LevelType get_level_type();
    private:
        unsigned int m_sample_count;
        unsigned int m_sample_index;
        AUDIOChannel::Sample *m_samples_p;
    };

///////////////////////////////////////////////////////////////////////////////
// public function declarations
///////////////////////////////////////////////////////////////////////////////

public:
    AUDIOProcessor();
    virtual ~AUDIOProcessor();

    void add_meter(Meter *meter_p);
    void clear_meter(AUDIOChannel *channel_p);
    const Meter *get_meter(const AUDIOChannel *channel_p) const;

    void add_handler(Handler *handler_p);
    void remove_handler(Handler *handler_p);

///////////////////////////////////////////////////////////////////////////////
// AUDIOCaptureManager::Handler declarations
///////////////////////////////////////////////////////////////////////////////

public:
    virtual ResultCode handle_samples(AUDIOChannel *channel_p, const size_t buffer_length, AUDIOChannel::Sample *buffer_p);

///////////////////////////////////////////////////////////////////////////////
// private function declarations
///////////////////////////////////////////////////////////////////////////////

private:
    static void timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents);

    Meter *find_meter_by_channel_index(AUDIOChannel::Index index) const;

///////////////////////////////////////////////////////////////////////////////
// private variable definitions
///////////////////////////////////////////////////////////////////////////////

private:
    struct ev_loop *m_loop_p;
    struct ev_timer m_timer;
    Handler *m_handler_p;
    std::map<AUDIOChannel::Index, Meter *> m_meters_map;

};

///////////////////////////////////////////////////////////////////////////////
// inline function implementations
///////////////////////////////////////////////////////////////////////////////

inline const AUDIOChannel *AUDIOProcessor::Meter::get_channel_p() const
{
    return m_channel_p;
}

inline uint32_t AUDIOProcessor::PeakMeter::get_hold_time() const
{
    return m_hold_time;
}

inline AUDIOChannel::Sample AUDIOProcessor::PeakMeter::get_peak() const
{
    return m_peak;
}

inline AUDIOChannel::Sample AUDIOProcessor::PeakMeter::get_hold() const
{
    return m_hold;
}

inline AUDIOProcessor::LevelType AUDIOProcessor::PPMMeter::get_level_type() 
{
    return LEVEL_TYPE_PPM;
}

inline AUDIOProcessor::LevelType AUDIOProcessor::DigitalPeakMeter::get_level_type() 
{
    return LEVEL_TYPE_DIGITALPEAK;
}

inline AUDIOProcessor::LevelType AUDIOProcessor::VUMeter::get_level_type()
{
    return LEVEL_TYPE_VU;
}

#endif
