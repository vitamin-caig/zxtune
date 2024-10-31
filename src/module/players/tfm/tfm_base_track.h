/**
 *
 * @file
 *
 * @brief  TFM-based track chiptunes support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/players/tfm/tfm_chiptune.h"
#include <module/players/tracking.h>

namespace Module::TFM
{
  const uint_t TRACK_CHANNELS = 6;

  class ChannelBuilder
  {
  public:
    ChannelBuilder(uint_t chan, Devices::TFM::Registers& data);

    void SetMode(uint_t mode);
    void KeyOn();
    void KeyOff();
    void SetKey(uint_t mask);
    void SetupConnection(uint_t algorithm, uint_t feedback);
    void SetDetuneMultiple(uint_t op, int_t detune, uint_t multiple);
    void SetRateScalingAttackRate(uint_t op, uint_t rateScaling, uint_t attackRate);
    void SetDecay(uint_t op, uint_t decay);
    void SetSustain(uint_t op, uint_t sustain);
    void SetSustainLevelReleaseRate(uint_t op, uint_t sustainLevel, uint_t releaseRate);
    void SetEnvelopeType(uint_t op, uint_t type);
    void SetTotalLevel(uint_t op, uint_t totalLevel);
    void SetTone(uint_t octave, uint_t tone);
    void SetTone(uint_t op, uint_t octave, uint_t tone);
    void SetPane(uint_t val);

  private:
    void WriteOperatorRegister(uint_t base, uint_t op, uint_t val);
    void WriteChannelRegister(uint_t base, uint_t val);
    void WriteChipRegister(uint_t idx, uint_t val);

  private:
    const uint_t Chip;
    const uint_t Channel;
    Devices::TFM::Registers& Registers;
  };

  class TrackBuilder
  {
  public:
    ChannelBuilder GetChannel(uint_t chan)
    {
      return {chan, Data};
    }

    void CaptureResult(Devices::TFM::Registers& res)
    {
      res.swap(Data);
    }

  private:
    Devices::TFM::Registers Data;
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
}  // namespace Module::TFM
