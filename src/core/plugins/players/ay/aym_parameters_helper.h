/*
Abstract:
  AYM parameters helper

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __AYM_PARAMETERS_HELPER_H_DEFINED__
#define __AYM_PARAMETERS_HELPER_H_DEFINED__

//common includes
#include <parameters.h>
//library includes
#include <core/freq_tables.h>
#include <devices/aym.h>
//std includes
#include <memory>

namespace ZXTune
{
  namespace AYM
  {
    Devices::AYM::ChipParameters::Ptr CreateChipParameters(Parameters::Accessor::Ptr params);

    class TrackParameters
    {
    public:
      typedef boost::shared_ptr<const TrackParameters> Ptr;

      virtual ~TrackParameters() {}

      virtual const Module::FrequencyTable& FreqTable() const = 0;

      static Ptr Create(Parameters::Accessor::Ptr params);
    };

    // Performs aym-related parameters applying
    class ParametersHelper
    {
    public:
      typedef boost::shared_ptr<ParametersHelper> Ptr;

      virtual ~ParametersHelper() {}

      //called with new parameters set
      virtual void SetParameters(const Parameters::Accessor& params) = 0;

      //frequency table according to parameters
      virtual const Module::FrequencyTable& GetFreqTable() const = 0;
      //initial data chunk according to parameters
      virtual void GetDataChunk(Devices::AYM::DataChunk& dst) const = 0;

      static Ptr Create();
    };

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
}

#endif //__AYM_PARAMETERS_HELPER_H_DEFINED__
