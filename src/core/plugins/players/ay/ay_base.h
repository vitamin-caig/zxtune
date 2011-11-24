/*
Abstract:
  AYM-based modules support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __CORE_PLUGINS_PLAYERS_AY_BASE_H_DEFINED__
#define __CORE_PLUGINS_PLAYERS_AY_BASE_H_DEFINED__

//local includes
#include "aym_parameters.h"
#include "core/plugins/players/module_properties.h"
#include "core/plugins/players/tracking.h"
//common includes
#include <error.h>
//library includes
#include <core/module_holder.h>
#include <devices/aym.h>

namespace ZXTune
{
  namespace Module
  {
    namespace AYM
    {
      class ChannelBuilder
      {
      public:
        ChannelBuilder(uint_t chan, const FrequencyTable& table, Devices::AYM::DataChunk& chunk)
          : Channel(chan)
          , Table(table)
          , Chunk(chunk)
        {
        }

        void SetTone(int_t halfTones, int_t offset);
        void SetLevel(int_t level);
        void DisableTone();
        void EnableEnvelope();
        void DisableNoise();
      private:
        const uint_t Channel;
        const FrequencyTable& Table;
        Devices::AYM::DataChunk& Chunk;
      };

      class TrackBuilder
      {
      public:
        explicit TrackBuilder(const FrequencyTable& table)
          : Table(table)
        {
          Chunk.Mask |= 1 << Devices::AYM::DataChunk::REG_MIXER;
        }

        void SetNoise(uint_t level);
        void SetEnvelopeType(uint_t type);
        void SetEnvelopeTone(uint_t tone);

        int_t GetSlidingDifference(int_t halfToneFrom, int_t halfToneTo) const;

        ChannelBuilder GetChannel(uint_t chan)
        {
          return ChannelBuilder(chan, Table, Chunk);
        }

        void GetResult(Devices::AYM::DataChunk& result) const
        {
          result = Chunk;
        }
      private:
        const FrequencyTable& Table;
        Devices::AYM::DataChunk Chunk;
      };

      typedef PatternCursorSet<Devices::AYM::CHANNELS> PatternCursors;

      class DataRenderer
      {
      public:
        typedef boost::shared_ptr<DataRenderer> Ptr;

        virtual ~DataRenderer() {}

        virtual void SynthesizeData(const TrackState& state, TrackBuilder& track) = 0;
        virtual void Reset() = 0;
      };

      class DataIterator : public StateIterator
      {
      public:
        typedef boost::shared_ptr<DataIterator> Ptr;

        virtual void GetData(Devices::AYM::DataChunk& chunk) const = 0;
      };

      class Chiptune
      {
      public:
        typedef boost::shared_ptr<const Chiptune> Ptr;
        virtual ~Chiptune() {}

        virtual Information::Ptr GetInformation() const = 0;
        virtual ModuleProperties::Ptr GetProperties() const = 0;
        virtual AYM::DataIterator::Ptr CreateDataIterator(TrackParameters::Ptr trackParams) const = 0;

        //TODO: temporary solution
        virtual Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Devices::AYM::Device::Ptr chip) const;
      };

      Analyzer::Ptr CreateAnalyzer(Devices::AYM::Device::Ptr device);

      DataIterator::Ptr CreateDataIterator(TrackParameters::Ptr trackParams, StateIterator::Ptr iterator, DataRenderer::Ptr renderer);

      Renderer::Ptr CreateRenderer(TrackParameters::Ptr trackParams, AYM::DataIterator::Ptr iterator, Devices::AYM::Device::Ptr device);

      Devices::AYM::Receiver::Ptr CreateReceiver(Sound::MultichannelReceiver::Ptr target);

      Holder::Ptr CreateHolder(Chiptune::Ptr chiptune);
    }
  }
}

#endif //__CORE_PLUGINS_PLAYERS_AY_BASE_H_DEFINED__
