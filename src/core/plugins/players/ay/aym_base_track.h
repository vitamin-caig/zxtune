/**
* 
* @file
*
* @brief  AYM-based track chiptunes support
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "aym_chiptune.h"
//library includes
#include <core/module_renderer.h>
#include <sound/render_params.h>

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

    DataIterator::Ptr CreateDataIterator(TrackParameters::Ptr trackParams, TrackStateIterator::Ptr iterator, DataRenderer::Ptr renderer);
  }
}
