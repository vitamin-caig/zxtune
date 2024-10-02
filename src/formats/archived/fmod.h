/**
 *
 * @file
 *
 * @brief  FMOD FSB format parser
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <string_view.h>
#include <types.h>
// library includes
#include <binary/container.h>
#include <binary/view.h>

namespace Formats::Archived::Fmod
{
  enum Format : uint_t
  {
    UNKNOWN,
    PCM8,
    PCM16,
    PCM24,
    PCM32,
    PCMFLOAT,
    GCADPCM,
    IMAADPCM,
    VAG,
    HEVAG,
    XMA,
    MPEG,
    CELT,
    AT9,
    XWMA,
    VORBIS,
    FADPCM,
  };

  enum ChunkType : uint_t
  {
    CHANNELS = 1,
    FREQUENCY = 2,
    LOOP = 3,
    XMASEEK = 6,
    DSPCOEFF = 7,
    ATRAC9DATA = 9,
    XWMADATA = 10,
    VORBISDATA = 11
  };

  class Builder
  {
  public:
    virtual ~Builder() = default;

    virtual void Setup(uint_t samplesCount, uint_t format) = 0;

    virtual void StartSample(uint_t idx) = 0;
    virtual void SetFrequency(uint_t frequency) = 0;
    virtual void SetChannels(uint_t channels) = 0;
    virtual void SetName(StringView name) = 0;

    virtual void AddMetaChunk(uint_t type, Binary::View chunk) = 0;
    virtual void SetData(uint_t samplesCount, Binary::Container::Ptr blob) = 0;
  };

  Binary::Container::Ptr Parse(const Binary::Container& rawData, Builder& target);
}  // namespace Formats::Archived::Fmod
