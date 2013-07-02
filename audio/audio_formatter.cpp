#include "audio_formatter.h"

#include <log4cxx/logger.h>

///////////////////////////////////////////////////////////////////////////////
// macros
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// type defintions
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// constants
///////////////////////////////////////////////////////////////////////////////

static const AUDIOChannel::Sample c_normalization_factor = (1.0 / 32768.0);

///////////////////////////////////////////////////////////////////////////////
// module variables
///////////////////////////////////////////////////////////////////////////////

static log4cxx::LoggerPtr g_logger(log4cxx::Logger::getLogger("audio.formatter"));


///////////////////////////////////////////////////////////////////////////////
// private function declarations
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// public function implementations
///////////////////////////////////////////////////////////////////////////////

AUDIOFormatter::~AUDIOFormatter()
{
    LOG4CXX_DEBUG(g_logger, "AUDIOFormatter::~AUDIOFormatter enter");
    LOG4CXX_DEBUG(g_logger, "AUDIOFormatter::~AUDIOFormatter exit");
}

AUDIOFormatter *AUDIOFormatterFactory::create_audio_formatter_p(snd_pcm_format_t format)
{
    LOG4CXX_DEBUG(g_logger, "AUDIOFormatterFactory::create_audi_formatter_p enter " << format);

    AUDIOFormatter *formatter_p = NULL;

    switch (format)
    {
    case SND_PCM_FORMAT_FLOAT_LE:
        formatter_p = new AUDIOFloatFormatter();
        break;
    case SND_PCM_FORMAT_S16_LE:
        formatter_p = new AUDIOSigned16BitFormatter();
        break;
    default:
        LOG4CXX_FATAL(g_logger, "unknown audio format=" << format);
        break;
    }

    LOG4CXX_DEBUG(g_logger, "AUDIOFormatterFactory::create_audio_formatter_p exit " << formatter_p);
    return formatter_p;
}

std::list<snd_pcm_format_t> AUDIOFormatterFactory::fetch_audio_format_list()
{
    LOG4CXX_DEBUG(g_logger, "AUDIOFormatterFactory::fetch_audio_format_list entry");

    std::list<snd_pcm_format_t> list;
    list.push_back(SND_PCM_FORMAT_FLOAT_LE);
    list.push_back(SND_PCM_FORMAT_S16_LE);

    LOG4CXX_DEBUG(g_logger, "AUDIOFormatterFactory::fetch_audio_format_list exit");

    return list;
}

AUDIOSigned16BitFormatter::AUDIOSigned16BitFormatter()
{
    LOG4CXX_DEBUG(g_logger, "AUDIOSigned16BitFormatter::AUDIOSigned16BitFormatter enter");
    LOG4CXX_DEBUG(g_logger, "AUDIOSigned16BitFormatter::AUDIOSigned16BitFormatter exit");
}

AUDIOSigned16BitFormatter::~AUDIOSigned16BitFormatter()
{
    LOG4CXX_DEBUG(g_logger, "AUDIOSigned16BitFormatter::~AUDIOSigned16BitFormatter enter");
    LOG4CXX_DEBUG(g_logger, "AUDIOSigned16BitFormatter::~AUDIOSigned16BitFormatter exit");
}

void AUDIOSigned16BitFormatter::format_samples(uint8_t *raw_buffer_p, AUDIOChannel::Index index, AUDIOChannel::Sample *sample_buffer_p, size_t num_samples)
{
    LOG4CXX_DEBUG(g_logger, "AUDIOSigned16BitFormatter::format_samples enter " << index << " " << num_samples);

    for (int counter = 0; counter < num_samples; counter++)
    {
        int16_t sample = ((int16_t *)raw_buffer_p)[(2 * counter) + index];
        sample_buffer_p[counter] = ((AUDIOChannel::Sample)sample) * c_normalization_factor;
    }

    LOG4CXX_DEBUG(g_logger, "AUDIOSigned16BitFormatter::format_sample exit");
}

AUDIOFloatFormatter::AUDIOFloatFormatter()
{
    LOG4CXX_DEBUG(g_logger, "AUDIOFloatFormatter::AUDIOFloatFormatter enter");
    LOG4CXX_DEBUG(g_logger, "AUDIOFloatFormatter::AUDIOFloatFormatter exit");
}

AUDIOFloatFormatter::~AUDIOFloatFormatter()
{
    LOG4CXX_DEBUG(g_logger, "AUDIOFloatFormatter::~AUDIOFloatFormatter enter");
    LOG4CXX_DEBUG(g_logger, "AUDIOFloatFormatter::~AUDIOFloatFormatter exit");
}

void AUDIOFloatFormatter::format_samples(uint8_t *raw_buffer_p, AUDIOChannel::Index index, AUDIOChannel::Sample *sample_buffer_p, size_t num_samples)
{
    LOG4CXX_DEBUG(g_logger, "AUDIOFloatFormatter::format_samples enter " << raw_buffer_p << " " << sample_buffer_p << " " << num_samples);

    for (int counter = 0; counter < num_samples; counter++)
    {
        float sample = ((float *)raw_buffer_p)[(2 * counter) + index];
        sample_buffer_p[counter] = sample;
    }

    LOG4CXX_DEBUG(g_logger, "AUDIOSigned16BitFormatter::format_sample exit");
}


///////////////////////////////////////////////////////////////////////////////
// protected function implementations
///////////////////////////////////////////////////////////////////////////////

AUDIOFormatter::AUDIOFormatter()
{
    LOG4CXX_DEBUG(g_logger, "AUDIOFormatter::AUDIOFormatter enter");
    LOG4CXX_DEBUG(g_logger, "AUDIOFormatter::AUDIOFormatter exit");
}

///////////////////////////////////////////////////////////////////////////////
// private function implementations
///////////////////////////////////////////////////////////////////////////////



