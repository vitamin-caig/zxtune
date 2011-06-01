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
#include <core/plugins/players/streaming.h>
#include <core/plugins/players/tracking.h>
//common includes
#include <error.h>
//library includes
#include <devices/aym.h>
#include <sound/render_params.h>

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
        }

        void SetNoise(uint_t level);
        void SetEnvelopeType(uint_t type);
        void SetEnvelopeTone(uint_t tone);

        void SetRawChunk(const Devices::AYM::DataChunk& chunk);

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

      Devices::AYM::Receiver::Ptr CreateReceiver(TrackParameters::Ptr params, Sound::MultichannelReceiver::Ptr target);

      Renderer::Ptr CreateRenderer(TrackParameters::Ptr params, StateIterator::Ptr iterator, DataRenderer::Ptr renderer, Devices::AYM::Chip::Ptr device);

      Renderer::Ptr CreateStreamRenderer(TrackParameters::Ptr params, Information::Ptr info, DataRenderer::Ptr renderer, Devices::AYM::Chip::Ptr device);

      Renderer::Ptr CreateTrackRenderer(TrackParameters::Ptr params, Information::Ptr info, TrackModuleData::Ptr data,
        DataRenderer::Ptr renderer, Devices::AYM::Chip::Ptr device);
    }
  }
}

#endif //__CORE_PLUGINS_PLAYERS_AY_BASE_H_DEFINED__
