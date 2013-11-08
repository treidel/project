#include "audio_formatter.h"
#include "log.h"

///////////////////////////////////////////////////////////////////////////////
// macros
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// type defintions
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// constants
///////////////////////////////////////////////////////////////////////////////

static const AUDIOChannel::Sample c_s16_normalization_factor = (1.0 / 32768.0);
static const AUDIOChannel::Sample c_s32_normalization_factor = (1.0 / 2147483648.0); 

///////////////////////////////////////////////////////////////////////////////
// module variables
///////////////////////////////////////////////////////////////////////////////

static LogInstance g_logger("audio.formatter");


///////////////////////////////////////////////////////////////////////////////
// private function declarations
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// public function implementations
///////////////////////////////////////////////////////////////////////////////

AUDIOFormatter::~AUDIOFormatter()
{
    LOG_GENERATE_TRACE(g_logger, "AUDIOFormatter::~AUDIOFormatter enter this=%p", this);
    LOG_GENERATE_TRACE(g_logger, "AUDIOFormatter::~AUDIOFormatter exit");
}

AUDIOFormatter *AUDIOFormatterFactory::create_audio_formatter_p(snd_pcm_format_t format)
{
    LOG_GENERATE_TRACE(g_logger, "AUDIOFormatterFactory::create_audi_formatter_p enter format=%d", format);

    AUDIOFormatter *formatter_p = NULL;

    switch (format)
    {
        case SND_PCM_FORMAT_FLOAT_LE:
            formatter_p = new AUDIOFloatFormatter();
            break;
        case SND_PCM_FORMAT_S32_LE:
            formatter_p = new AUDIOSigned32BitFormatter();
            break;
        case SND_PCM_FORMAT_S16_LE:
            formatter_p = new AUDIOSigned16BitFormatter();
            break;
        default:
            LOG_GENERATE_WTF(g_logger, "unknown audio format=%d", format);
            break;
    }

    LOG_GENERATE_TRACE(g_logger, "AUDIOFormatterFactory::create_audio_formatter_p exit formatter_p=%p", formatter_p);
    return formatter_p;
}

std::list<snd_pcm_format_t> AUDIOFormatterFactory::fetch_audio_format_list()
{
    LOG_GENERATE_TRACE(g_logger, "AUDIOFormatterFactory::fetch_audio_format_list entry");

    std::list<snd_pcm_format_t> list;
    list.push_back(SND_PCM_FORMAT_FLOAT_LE);
    list.push_back(SND_PCM_FORMAT_S32_LE);
    list.push_back(SND_PCM_FORMAT_S16_LE);

    LOG_GENERATE_TRACE(g_logger, "AUDIOFormatterFactory::fetch_audio_format_list exit");

    return list;
}

AUDIOSigned16BitFormatter::AUDIOSigned16BitFormatter()
{
    LOG_GENERATE_TRACE(g_logger, "AUDIOSigned16BitFormatter::AUDIOSigned16BitFormatter enter this=%p", this);
    LOG_GENERATE_TRACE(g_logger, "AUDIOSigned16BitFormatter::AUDIOSigned16BitFormatter exit");
}

AUDIOSigned16BitFormatter::~AUDIOSigned16BitFormatter()
{
    LOG_GENERATE_TRACE(g_logger, "AUDIOSigned16BitFormatter::~AUDIOSigned16BitFormatter enter this=%p", this);
    LOG_GENERATE_TRACE(g_logger, "AUDIOSigned16BitFormatter::~AUDIOSigned16BitFormatter exit");
}

void AUDIOSigned16BitFormatter::format_samples(uint8_t *raw_buffer_p, AUDIOChannel::Index index, AUDIOChannel::Sample *sample_buffer_p, size_t num_samples)
{
    LOG_GENERATE_TRACE(g_logger, "AUDIOSigned16BitFormatter::format_samples enter this=%p raw_buffer_p=%p index=%d sample_buffer_p=%p num_samples=%d", this, raw_buffer_p, index, sample_buffer_p, num_samples);

    for (int counter = 0; counter < num_samples; counter++)
    {
        int16_t sample = ((int16_t *)raw_buffer_p)[(2 * counter) + index];
        sample_buffer_p[counter] = ((AUDIOChannel::Sample)sample) * c_s16_normalization_factor;
    }

    LOG_GENERATE_TRACE(g_logger, "AUDIOSigned16BitFormatter::format_sample exit");
}

AUDIOSigned32BitFormatter::AUDIOSigned32BitFormatter()
{
    LOG_GENERATE_TRACE(g_logger, "AUDIOSigned32BitFormatter::AUDIOSigned32BitFormatter enter this=%p", this);
    LOG_GENERATE_TRACE(g_logger, "AUDIOSigned32BitFormatter::AUDIOSigned32BitFormatter exit");
}

AUDIOSigned32BitFormatter::~AUDIOSigned32BitFormatter()
{
    LOG_GENERATE_TRACE(g_logger, "AUDIOSigned32BitFormatter::~AUDIOSigned32BitFormatter enter this=%p", this);
    LOG_GENERATE_TRACE(g_logger, "AUDIOSigned32BitFormatter::~AUDIOSigned32BitFormatter exit");
}

void AUDIOSigned32BitFormatter::format_samples(uint8_t *raw_buffer_p, AUDIOChannel::Index index, AUDIOChannel::Sample *sample_buffer_p, size_t num_samples)
{
    LOG_GENERATE_TRACE(g_logger, "AUDIOSigned32BitFormatter::format_samples enter this=%p raw_buffer_p=%p index=%d sample_buffer_p=%p num_samples=%d", this, raw_buffer_p, index, sample_buffer_p, num_samples);

    for (int counter = 0; counter < num_samples; counter++)
    {
        int32_t sample = ((int32_t *)raw_buffer_p)[(2 * counter) + index];
        sample_buffer_p[counter] = ((AUDIOChannel::Sample)sample) * c_s32_normalization_factor;
    }

    LOG_GENERATE_TRACE(g_logger, "AUDIOSigned16BitFormatter::format_sample exit");
}

AUDIOFloatFormatter::AUDIOFloatFormatter()
{
    LOG_GENERATE_TRACE(g_logger, "AUDIOFloatFormatter::AUDIOFloatFormatter enter this=%p", this);
    LOG_GENERATE_TRACE(g_logger, "AUDIOFloatFormatter::AUDIOFloatFormatter exit");
}

AUDIOFloatFormatter::~AUDIOFloatFormatter()
{
    LOG_GENERATE_TRACE(g_logger, "AUDIOFloatFormatter::~AUDIOFloatFormatter enter this=%p", this);
    LOG_GENERATE_TRACE(g_logger, "AUDIOFloatFormatter::~AUDIOFloatFormatter exit");
}

void AUDIOFloatFormatter::format_samples(uint8_t *raw_buffer_p, AUDIOChannel::Index index, AUDIOChannel::Sample *sample_buffer_p, size_t num_samples)
{
    LOG_GENERATE_TRACE(g_logger, "AUDIOFloatFormatter::format_samples enter this=%p raw_buffer_p=%p index=%d sample_buffer_p=%p num_samples=%d", this, raw_buffer_p, index, sample_buffer_p, num_samples);

    for (int counter = 0; counter < num_samples; counter++)
    {
        float sample = ((float *)raw_buffer_p)[(2 * counter) + index];
        sample_buffer_p[counter] = sample;
    }

    LOG_GENERATE_TRACE(g_logger, "AUDIOSigned16BitFormatter::format_sample exit");
}


///////////////////////////////////////////////////////////////////////////////
// protected function implementations
///////////////////////////////////////////////////////////////////////////////

AUDIOFormatter::AUDIOFormatter()
{
    LOG_GENERATE_TRACE(g_logger, "AUDIOFormatter::AUDIOFormatter enter this=%p", this);
    LOG_GENERATE_TRACE(g_logger, "AUDIOFormatter::AUDIOFormatter exit");
}

///////////////////////////////////////////////////////////////////////////////
// private function implementations
///////////////////////////////////////////////////////////////////////////////



