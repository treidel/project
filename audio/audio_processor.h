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

    typedef struct
    {
        AUDIOChannel::Index channel;
        int32_t levelInDB;
    } ResultData;

    class Handler
    {
    public:
        virtual ResultCode handle_results(const size_t num_results, const ResultData results[]) = 0;
    };

    typedef enum
    {
        LEVEL_TYPE_NONE = 0,
        LEVEL_TYPE_PEAK = 1,
        LEVEL_TYPE_VU = 2
    } LevelType;

private:

    class Meter
    {
    public:
        virtual ~Meter();
        virtual void process_samples(const size_t buffer_length, AUDIOChannel::Sample *buffer_p) = 0;
        virtual ResultData create_result_data() = 0;
        inline const AUDIOChannel *get_channel_p() const;
    protected:
        Meter(AUDIOChannel *channel_p);
    private:
        AUDIOChannel *m_channel_p;
    };

    class PeakMeter : public Meter
    {
    public:
        PeakMeter(AUDIOChannel *channel_p);
        virtual ~PeakMeter();
        void process_samples(const size_t buffer_length, AUDIOChannel::Sample *buffer_p);
        ResultData create_result_data();
    private:
        AUDIOChannel::Sample m_peak;
    };

    class VUMeter : public Meter
    {
    public:
        VUMeter(AUDIOChannel *channel_p);
        virtual ~VUMeter();
        void process_samples(const size_t buffer_length, AUDIOChannel::Sample *buffer_p);
        ResultData create_result_data();
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

    void set_level_type(LevelType level_type);
    inline LevelType get_level_type() const;

    void add_handler(Handler *handler_p);
    void remove_handler(Handler *handler_p);

///////////////////////////////////////////////////////////////////////////////
// AUDIOCaptureManager::Handler declarations
///////////////////////////////////////////////////////////////////////////////

public:
    virtual ResultCode handle_samples(AUDIOChannel::Index index, const size_t buffer_length, AUDIOChannel::Sample *buffer_p);

///////////////////////////////////////////////////////////////////////////////
// private function declarations
///////////////////////////////////////////////////////////////////////////////

private:
    static void timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents);


///////////////////////////////////////////////////////////////////////////////
// private variable definitions
///////////////////////////////////////////////////////////////////////////////

private:
    struct ev_loop *m_loop_p;
    struct ev_timer m_timer;
    Handler *m_handler_p;
    std::map<AUDIOChannel::Index, Meter *> m_meters_map;
    LevelType m_level_type;

};

///////////////////////////////////////////////////////////////////////////////
// inline function implementations
///////////////////////////////////////////////////////////////////////////////

inline AUDIOProcessor::LevelType AUDIOProcessor::get_level_type() const
{
    return m_level_type;
}

inline const AUDIOChannel *AUDIOProcessor::Meter::get_channel_p() const
{
    return m_channel_p;
}

#endif
