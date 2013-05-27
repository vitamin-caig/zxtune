/*
Abstract:
  TFM-based players common functionality

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef CORE_PLUGINS_PLAYERS_TFM_BASE_DEFINED
#define CORE_PLUGINS_PLAYERS_TFM_BASE_DEFINED

//local includes
#include "tfm_parameters.h"
#include "core/plugins/players/module_properties.h"
#include "core/plugins/players/tracking.h"
//library includes
#include <core/module_holder.h>

namespace ZXTune
{
  namespace Module
  {
    namespace TFM
    {
      const uint_t TRACK_CHANNELS = 6;

      class ChannelBuilder
      {
      public:
        ChannelBuilder(uint_t chan, Devices::TFM::DataChunk& chunk);

        void KeyOn();
        void KeyOff();
        void SetupConnection(uint_t algorithm, uint_t feedback);
        void SetDetuneMultiple(uint_t op, int_t detune, uint_t multiple);
        void SetRateScalingAttackRate(uint_t op, uint_t rateScaling, uint_t attackRate);
        void SetDecay(uint_t op, uint_t decay);
        void SetSustain(uint_t op, uint_t sustain);
        void SetSustainLevelReleaseRate(uint_t op, uint_t sustainLevel, uint_t releaseRate);
        void SetEnvelopeType(uint_t op, uint_t type);
        void SetTotalLevel(uint_t op, uint_t totalLevel);
        void SetTone(uint_t octave, uint_t tone);
      private:
        void WriteOperatorRegister(uint_t base, uint_t op, uint_t val);
        void WriteChannelRegister(uint_t base, uint_t val);
        void WriteChipRegister(uint_t idx, uint_t val);
      private:
        const uint_t Channel;
        Devices::FM::DataChunk::Registers& Registers;
      };

      class TrackBuilder
      {
      public:
        ChannelBuilder GetChannel(uint_t chan)
        {
          return ChannelBuilder(chan, Chunk);
        }

        void GetResult(Devices::TFM::DataChunk& result) const
        {
          result = Chunk;
        }
      private:
        Devices::TFM::DataChunk Chunk;
      };

      class DataRenderer
      {
      public:
        typedef boost::shared_ptr<DataRenderer> Ptr;

        virtual ~DataRenderer() {}

        virtual void SynthesizeData(const TrackModelState& state, TrackBuilder& track) = 0;
        virtual void Reset() = 0;
      };

      class DataIterator : public StateIterator
      {
      public:
        typedef boost::shared_ptr<DataIterator> Ptr;

        virtual Devices::TFM::DataChunk GetData() const = 0;
      };

      class Chiptune
      {
      public:
        typedef boost::shared_ptr<const Chiptune> Ptr;
        virtual ~Chiptune() {}

        virtual Information::Ptr GetInformation() const = 0;
        virtual ModuleProperties::Ptr GetProperties() const = 0;
        virtual DataIterator::Ptr CreateDataIterator(TrackParameters::Ptr params) const = 0;

        virtual Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Devices::TFM::Device::Ptr chip) const;
      };

      Analyzer::Ptr CreateAnalyzer(Devices::TFM::Device::Ptr device);

      DataIterator::Ptr CreateDataIterator(TFM::TrackParameters::Ptr trackParams, TrackStateIterator::Ptr iterator, DataRenderer::Ptr renderer);

      Renderer::Ptr CreateRenderer(TrackParameters::Ptr trackParams, DataIterator::Ptr iterator, Devices::TFM::Device::Ptr device);

      Devices::TFM::Receiver::Ptr CreateReceiver(Sound::OneChannelReceiver::Ptr target);

      Holder::Ptr CreateHolder(Chiptune::Ptr chiptune);
    }
  }
}

#endif //CORE_PLUGINS_PLAYERS_FM_BASE_DEFINED
