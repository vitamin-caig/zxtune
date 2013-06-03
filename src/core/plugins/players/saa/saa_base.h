/*
Abstract:
  SAA-based players common functionality

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef CORE_PLUGINS_PLAYERS_SAA_BASE_DEFINED
#define CORE_PLUGINS_PLAYERS_SAA_BASE_DEFINED

//local includes
#include "saa_parameters.h"
#include "core/plugins/players/module_properties.h"
#include "core/plugins/players/tracking.h"
//library includes
#include <core/module_holder.h>

namespace ZXTune
{
  namespace Module
  {
    namespace SAA
    {
      const uint_t TRACK_CHANNELS = 6;

      class ChannelBuilder
      {
      public:
        ChannelBuilder(uint_t chan, Devices::SAA::DataChunk& chunk);

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
          Chunk.Data[reg] = static_cast<uint8_t>(val);
          Chunk.Mask |= 1 << reg;
        }

        void SetRegister(uint_t reg, uint_t val, uint_t mask)
        {
          Chunk.Data[reg] &= ~mask;
          AddRegister(reg, val);
        }

        void AddRegister(uint_t reg, uint_t val)
        {
          Chunk.Data[reg] |= static_cast<uint8_t>(val);
          Chunk.Mask |= 1 << reg;
        }
      private:
        const uint_t Channel;
        Devices::SAA::DataChunk& Chunk;
      };

      class TrackBuilder
      {
      public:
        ChannelBuilder GetChannel(uint_t chan)
        {
          return ChannelBuilder(chan, Chunk);
        }

        void GetResult(Devices::SAA::DataChunk& result) const
        {
          result = Chunk;
        }
      private:
        Devices::SAA::DataChunk Chunk;
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

        virtual Devices::SAA::DataChunk GetData() const = 0;
      };

      class Chiptune
      {
      public:
        typedef boost::shared_ptr<const Chiptune> Ptr;
        virtual ~Chiptune() {}

        virtual Information::Ptr GetInformation() const = 0;
        virtual ModuleProperties::Ptr GetProperties() const = 0;
        virtual DataIterator::Ptr CreateDataIterator(TrackParameters::Ptr params) const = 0;

        virtual Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Devices::SAA::Device::Ptr chip) const;
      };

      Analyzer::Ptr CreateAnalyzer(Devices::SAA::Device::Ptr device);

      DataIterator::Ptr CreateDataIterator(TrackParameters::Ptr trackParams, TrackStateIterator::Ptr iterator, DataRenderer::Ptr renderer);

      Renderer::Ptr CreateRenderer(TrackParameters::Ptr trackParams, DataIterator::Ptr iterator, Devices::SAA::Device::Ptr device);

      Holder::Ptr CreateHolder(Chiptune::Ptr chiptune);
    }
  }
}

#endif //CORE_PLUGINS_PLAYERS_SAA_BASE_DEFINED
