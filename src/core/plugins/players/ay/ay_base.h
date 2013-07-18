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
#include "core/plugins/players/tracking.h"
//common includes
#include <error.h>
//library includes
#include <core/module_holder.h>
#include <devices/aym.h>
#include <sound/render_params.h>

namespace ZXTune
{
  namespace Module
  {
    namespace AYM
    {
      const uint_t TRACK_CHANNELS = 3;
     
      class ChannelBuilder
      {
      public:
        ChannelBuilder(uint_t chan, const FrequencyTable& table, Devices::AYM::Registers& data)
          : Channel(chan)
          , Table(table)
          , Data(data)
        {
        }

        void SetTone(int_t halfTones, int_t offset);
        void SetTone(uint_t tone);
        void SetLevel(int_t level);
        void DisableTone();
        void EnableEnvelope();
        void DisableNoise();
      private:
        const uint_t Channel;
        const FrequencyTable& Table;
        Devices::AYM::Registers& Data;
      };

      class TrackBuilder
      {
      public:
        explicit TrackBuilder(const FrequencyTable& table)
          : Table(table)
        {
          Data[Devices::AYM::Registers::MIXER] = 0;
        }

        void SetNoise(uint_t level);
        void SetEnvelopeType(uint_t type);
        void SetEnvelopeTone(uint_t tone);

        uint_t GetFrequency(int_t halfTone) const;
        int_t GetSlidingDifference(int_t halfToneFrom, int_t halfToneTo) const;

        ChannelBuilder GetChannel(uint_t chan)
        {
          return ChannelBuilder(chan, Table, Data);
        }

        const Devices::AYM::Registers& GetResult() const
        {
          return Data;
        }
      private:
        const FrequencyTable& Table;
        Devices::AYM::Registers Data;
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

        virtual Devices::AYM::Registers GetData() const = 0;
      };

      class Chiptune
      {
      public:
        typedef boost::shared_ptr<const Chiptune> Ptr;
        virtual ~Chiptune() {}

        virtual Information::Ptr GetInformation() const = 0;
        virtual Parameters::Accessor::Ptr GetProperties() const = 0;
        virtual AYM::DataIterator::Ptr CreateDataIterator(TrackParameters::Ptr trackParams) const = 0;
      };

      //TODO: refactor
      class Holder : public ZXTune::Module::Holder
      {
      public:
        typedef boost::shared_ptr<const Holder> Ptr;

        virtual Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Devices::AYM::Device::Ptr chip) const = 0;
      };

      Analyzer::Ptr CreateAnalyzer(Devices::AYM::Device::Ptr device);

      DataIterator::Ptr CreateDataIterator(TrackParameters::Ptr trackParams, TrackStateIterator::Ptr iterator, DataRenderer::Ptr renderer);

      Renderer::Ptr CreateRenderer(Sound::RenderParameters::Ptr params, AYM::DataIterator::Ptr iterator, Devices::AYM::Device::Ptr device);
      Renderer::Ptr CreateRenderer(const Holder& holder, Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target);

      Holder::Ptr CreateHolder(Chiptune::Ptr chiptune);

      typedef std::vector<Devices::AYM::Registers> RegistersArray;
      typedef boost::shared_ptr<const RegistersArray> RegistersArrayPtr;

      Chiptune::Ptr CreateStreamedChiptune(RegistersArrayPtr data, Parameters::Accessor::Ptr properties, uint_t loopFrame);
    }
  }
}

#endif //__CORE_PLUGINS_PLAYERS_AY_BASE_H_DEFINED__
