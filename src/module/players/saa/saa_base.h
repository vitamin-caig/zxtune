/**
 *
 * @file
 *
 * @brief  SAA-based chiptunes support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/players/saa/saa_parameters.h"
#include "module/players/tracking.h"

#include "module/holder.h"

namespace Module::SAA
{
  const uint_t TRACK_CHANNELS = 6;
  const auto BASE_FRAME_DURATION = Time::Microseconds::FromFrequency(50);

  class ChannelBuilder
  {
  public:
    ChannelBuilder(uint_t chan, Devices::SAA::Registers& regs);

    void SetVolume(int_t left, int_t right);
    void SetTone(uint_t octave, uint_t note);
    void SetNoise(uint_t type);
    void AddNoise(uint_t type);
    void SetEnvelope(uint_t type);
    void EnableTone();
    void EnableNoise();

  private:
    void SetRegister(uint_t reg, uint_t val)
    {
      Regs.Data[reg] = static_cast<uint8_t>(val);
      Regs.Mask |= 1 << reg;
    }

    void SetRegister(uint_t reg, uint_t val, uint_t mask)
    {
      Regs.Data[reg] &= ~mask;
      AddRegister(reg, val);
    }

    void AddRegister(uint_t reg, uint_t val)
    {
      Regs.Data[reg] |= static_cast<uint8_t>(val);
      Regs.Mask |= 1 << reg;
    }

  private:
    const uint_t Channel;
    Devices::SAA::Registers& Regs;
  };

  class TrackBuilder
  {
  public:
    ChannelBuilder GetChannel(uint_t chan)
    {
      return {chan, Regs};
    }

    void GetResult(Devices::SAA::Registers& result) const
    {
      result = Regs;
    }

  private:
    Devices::SAA::Registers Regs;
  };

  class DataRenderer
  {
  public:
    using Ptr = std::unique_ptr<DataRenderer>;

    virtual ~DataRenderer() = default;

    virtual void SynthesizeData(const TrackModelState& state, TrackBuilder& track) = 0;
    virtual void Reset() = 0;
  };

  class DataIterator : public Iterator
  {
  public:
    using Ptr = std::unique_ptr<DataIterator>;

    virtual State::Ptr GetStateObserver() const = 0;

    virtual Devices::SAA::Registers GetData() const = 0;
  };

  class Chiptune
  {
  public:
    using Ptr = std::unique_ptr<const Chiptune>;
    virtual ~Chiptune() = default;

    virtual Time::Microseconds GetFrameDuration() const = 0;

    virtual TrackModel::Ptr GetTrackModel() const = 0;
    virtual Parameters::Accessor::Ptr GetProperties() const = 0;
    virtual DataIterator::Ptr CreateDataIterator() const = 0;
  };

  DataIterator::Ptr CreateDataIterator(TrackStateIterator::Ptr iterator, DataRenderer::Ptr renderer);

  Holder::Ptr CreateHolder(Chiptune::Ptr chiptune);
}  // namespace Module::SAA
