/**
 *
 * @file
 *
 * @brief  DAC-based modules support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/players/dac/dac_chiptune.h"
#include <module/players/tracking.h>

#include <module/renderer.h>

namespace Module::DAC
{
  class ChannelDataBuilder
  {
  public:
    explicit ChannelDataBuilder(Devices::DAC::ChannelData& data)
      : Data(data)
    {}

    void SetEnabled(bool enabled)
    {
      Data.Enabled = enabled;
      Data.Mask |= Devices::DAC::ChannelData::ENABLED;
    }

    void SetNote(uint_t note)
    {
      Data.Note = note;
      Data.Mask |= Devices::DAC::ChannelData::NOTE;
    }

    void SetNoteSlide(int_t noteSlide)
    {
      Data.NoteSlide = noteSlide;
      Data.Mask |= Devices::DAC::ChannelData::NOTESLIDE;
    }

    void SetFreqSlideHz(int_t freqSlideHz)
    {
      Data.FreqSlideHz = freqSlideHz;
      Data.Mask |= Devices::DAC::ChannelData::FREQSLIDEHZ;
    }

    void SetSampleNum(uint_t sampleNum)
    {
      Data.SampleNum = sampleNum;
      Data.Mask |= Devices::DAC::ChannelData::SAMPLENUM;
    }

    void SetPosInSample(uint_t posInSample)
    {
      Data.PosInSample = posInSample;
      Data.Mask |= Devices::DAC::ChannelData::POSINSAMPLE;
    }

    void DropPosInSample()
    {
      Data.Mask &= ~Devices::DAC::ChannelData::POSINSAMPLE;
    }

    void SetLevelInPercents(uint_t levelInPercents)
    {
      Data.Level =
          Devices::DAC::ChannelData::LevelType(levelInPercents, Devices::DAC::ChannelData::LevelType::PRECISION);
      Data.Mask |= Devices::DAC::ChannelData::LEVEL;
    }

    Devices::DAC::ChannelData& GetState() const
    {
      return Data;
    }

  private:
    Devices::DAC::ChannelData& Data;
  };

  class TrackBuilder
  {
  public:
    ChannelDataBuilder GetChannel(uint_t chan);

    void GetResult(Devices::DAC::Channels& result);

  private:
    Devices::DAC::Channels Data;
  };

  class DataRenderer
  {
  public:
    using Ptr = std::unique_ptr<DataRenderer>;

    virtual ~DataRenderer() = default;

    virtual void SynthesizeData(const TrackModelState& state, TrackBuilder& track) = 0;
    virtual void Reset() = 0;
  };

  DataIterator::Ptr CreateDataIterator(TrackStateIterator::Ptr iterator, DataRenderer::Ptr renderer);

  Renderer::Ptr CreateRenderer(Time::Microseconds frameDuration, DataIterator::Ptr iterator,
                               Devices::DAC::Chip::Ptr device);
}  // namespace Module::DAC
