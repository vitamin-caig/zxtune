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
      virtual void GetDataChunk(DataChunk& dst) const = 0;

      static Ptr Create(const String& defaultFreqTable);
    };
  }

  //Temporary adapters
  class AYMChannelSynthesizer
  {
  public:
    AYMChannelSynthesizer(uint_t chan, const AYM::ParametersHelper& helper, AYM::DataChunk& chunk)
      : Channel(chan)
      , Helper(helper)
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
    const AYM::ParametersHelper& Helper;
    AYM::DataChunk& Chunk;
  };

  class AYMTrackSynthesizer
  {
  public:
    explicit AYMTrackSynthesizer(const AYM::ParametersHelper& helper)
      : Helper(helper)
    {
    }

    //infrastructure
    void InitData(uint64_t tickToPlay);
    const AYM::DataChunk& GetData() const;

    void SetNoise(uint_t level);
    void SetEnvelopeType(uint_t type);
    void SetEnvelopeTone(uint_t tone);

    void SetRawChunk(const AYM::DataChunk& chunk);

    int_t GetSlidingDifference(int_t halfToneFrom, int_t halfToneTo);

    AYMChannelSynthesizer GetChannel(uint_t chan)
    {
      return AYMChannelSynthesizer(chan, Helper, Chunk);
    }
  private:
    const AYM::ParametersHelper& Helper;
    AYM::DataChunk Chunk;
  };
}

#endif //__AYM_PARAMETERS_HELPER_H_DEFINED__
