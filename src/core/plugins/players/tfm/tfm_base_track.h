/*
Abstract:
  TFM-based track players common functionality

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef CORE_PLUGINS_PLAYERS_TFM_BASE_TRACK_DEFINED
#define CORE_PLUGINS_PLAYERS_TFM_BASE_TRACK_DEFINED

//local includes
#include "tfm_chiptune.h"
#include "core/plugins/players/renderer.h"
//library includes
#include <sound/render_params.h>

namespace Module
{
  namespace TFM
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
      const uint_t Channel;
      Devices::FM::Registers& Registers;
    };

    class TrackBuilder
    {
    public:
      ChannelBuilder GetChannel(uint_t chan)
      {
        return ChannelBuilder(chan, Data);
      }

      const Devices::TFM::Registers& GetResult() const
      {
        return Data;
      }
    private:
      Devices::TFM::Registers Data;
    };

    class DataRenderer
    {
    public:
      typedef boost::shared_ptr<DataRenderer> Ptr;

      virtual ~DataRenderer() {}

      virtual void SynthesizeData(const TrackModelState& state, TrackBuilder& track) = 0;
      virtual void Reset() = 0;
    };

    DataIterator::Ptr CreateDataIterator(TrackStateIterator::Ptr iterator, DataRenderer::Ptr renderer);
  }
}

#endif //CORE_PLUGINS_PLAYERS_TFM_BASE_TRACK_DEFINED
