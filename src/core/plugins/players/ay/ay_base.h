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
  namespace AYM
  {
    class ChannelBuilder
    {
    public:
      ChannelBuilder(uint_t chan, const Module::FrequencyTable& table, Devices::AYM::DataChunk& chunk)
        : Channel(chan)
        , Table(table)
        , Chunk(chunk)
      {
      }

      void SetTone(int_t halfTones, int_t offset) const;
      void SetLevel(int_t level) const;
      void DisableTone() const;
      void EnableEnvelope() const;
      void DisableNoise() const;
    private:
      const uint_t Channel;
      const Module::FrequencyTable& Table;
      Devices::AYM::DataChunk& Chunk;
    };

    class TrackBuilder
    {
    public:
      explicit TrackBuilder(const Module::FrequencyTable& table, Devices::AYM::DataChunk& chunk)
        : Table(table)
        , Chunk(chunk)
      {
      }

      void SetNoise(uint_t level) const;
      void SetEnvelopeType(uint_t type) const;
      void SetEnvelopeTone(uint_t tone) const;

      void SetRawChunk(const Devices::AYM::DataChunk& chunk) const;

      int_t GetSlidingDifference(int_t halfToneFrom, int_t halfToneTo) const;

      ChannelBuilder GetChannel(uint_t chan) const
      {
        return ChannelBuilder(chan, Table, Chunk);
      }
    private:
      const Module::FrequencyTable& Table;
      Devices::AYM::DataChunk& Chunk;
    };
  }

  namespace Module
  {
    typedef PatternCursorSet<Devices::AYM::CHANNELS> AYMPatternCursors;

    class AYMDataRenderer
    {
    public:
      typedef boost::shared_ptr<AYMDataRenderer> Ptr;

      virtual ~AYMDataRenderer() {}

      virtual void SynthesizeData(const TrackState& state, const AYM::TrackBuilder& track) = 0;
      virtual void Reset() = 0;
    };

    Devices::AYM::Receiver::Ptr CreateAYMReceiver(AYM::TrackParameters::Ptr params, Sound::MultichannelReceiver::Ptr target);

    Renderer::Ptr CreateAYMRenderer(AYM::TrackParameters::Ptr params, StateIterator::Ptr iterator, AYMDataRenderer::Ptr renderer, Devices::AYM::Chip::Ptr device);

    Renderer::Ptr CreateAYMStreamRenderer(AYM::TrackParameters::Ptr params, Information::Ptr info, AYMDataRenderer::Ptr renderer, Devices::AYM::Chip::Ptr device);

    Renderer::Ptr CreateAYMTrackRenderer(AYM::TrackParameters::Ptr params, Information::Ptr info, TrackModuleData::Ptr data,
      AYMDataRenderer::Ptr renderer, Devices::AYM::Chip::Ptr device);
  }
}

#endif //__CORE_PLUGINS_PLAYERS_AY_BASE_H_DEFINED__
