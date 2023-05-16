/**
 *
 * @file
 *
 * @brief  DAC-based devices support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// library includes
#include <devices/dac/sample.h>
#include <math/fixedpoint.h>
#include <sound/mixer.h>
#include <time/instant.h>

// supporting for multichannel sample-based DAC
namespace Devices::DAC
{
  using TimeUnit = Time::Microsecond;
  using Stamp = Time::Instant<TimeUnit>;

  struct ChannelData
  {
    ChannelData()
      : Channel()
      , Mask()
      , Enabled()
      , Note()
      , NoteSlide()
      , FreqSlideHz()
      , SampleNum()
      , PosInSample()
      , Level()
    {}

    enum Flags
    {
      ENABLED = 1,
      NOTE = 2,
      NOTESLIDE = 4,
      FREQSLIDEHZ = 8,
      SAMPLENUM = 16,
      POSINSAMPLE = 32,
      LEVEL = 64,

      ALL_PARAMETERS = 127
    };

    using LevelType = Math::FixedPoint<uint8_t, 100>;

    uint_t Channel;
    uint_t Mask;
    bool Enabled;
    uint_t Note;
    int_t NoteSlide;
    int_t FreqSlideHz;
    uint_t SampleNum;
    uint_t PosInSample;
    LevelType Level;

    const bool* GetEnabled() const
    {
      return 0 != (Mask & ENABLED) ? &Enabled : nullptr;
    }

    const uint_t* GetNote() const
    {
      return 0 != (Mask & NOTE) ? &Note : nullptr;
    }

    const int_t* GetNoteSlide() const
    {
      return 0 != (Mask & NOTESLIDE) ? &NoteSlide : nullptr;
    }

    const int_t* GetFreqSlideHz() const
    {
      return 0 != (Mask & FREQSLIDEHZ) ? &FreqSlideHz : nullptr;
    }

    const uint_t* GetSampleNum() const
    {
      return 0 != (Mask & SAMPLENUM) ? &SampleNum : nullptr;
    }

    const uint_t* GetPosInSample() const
    {
      return 0 != (Mask & POSINSAMPLE) ? &PosInSample : nullptr;
    }

    const LevelType* GetLevel() const
    {
      return 0 != (Mask & LEVEL) ? &Level : nullptr;
    }
  };

  typedef std::vector<ChannelData> Channels;

  struct DataChunk
  {
    Stamp TimeStamp;
    Channels Data;
  };

  class Chip
  {
  public:
    typedef std::shared_ptr<Chip> Ptr;

    virtual ~Chip() = default;

    /// Set sample for work
    virtual void SetSample(uint_t idx, Sample::Ptr sample) = 0;

    /// Render single data chunk
    virtual void RenderData(const DataChunk& src) = 0;

    /// Same as RenderData but do not produce sound output
    virtual void UpdateState(const DataChunk& src) = 0;

    /// reset internal state to initial
    virtual void Reset() = 0;

    /// Render rest data and return result
    virtual Sound::Chunk RenderTill(Stamp stamp) = 0;
  };

  class ChipParameters
  {
  public:
    typedef std::shared_ptr<const ChipParameters> Ptr;

    virtual ~ChipParameters() = default;

    virtual uint_t Version() const = 0;
    virtual uint_t BaseSampleFreq() const = 0;
    virtual uint_t SoundFreq() const = 0;
    virtual bool Interpolate() const = 0;
  };

  /// Virtual constructors
  Chip::Ptr CreateChip(ChipParameters::Ptr params, Sound::ThreeChannelsMixer::Ptr mixer);
  Chip::Ptr CreateChip(ChipParameters::Ptr params, Sound::FourChannelsMixer::Ptr mixer);
}  // namespace Devices::DAC
