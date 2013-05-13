/*
Abstract:
  ETracker format description

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef FORMATS_CHIPTUNE_ETRACKER_H_DEFINED
#define FORMATS_CHIPTUNE_ETRACKER_H_DEFINED

//local includes
#include "builder_meta.h"
#include "builder_pattern.h"
//common includes
#include <binary/container.h>
//library includes
#include <formats/chiptune.h>

namespace Formats
{
  namespace Chiptune
  {
    namespace ETracker
    {
      struct Sample
      {
        struct Line
        {
          Line() : LeftLevel(), RightLevel(), ToneEnabled(), NoiseEnabled(), ToneDeviation(), NoiseFreq()
          {
          }

          uint_t LeftLevel;//0-15
          uint_t RightLevel;//0-15
          bool ToneEnabled;
          bool NoiseEnabled;
          uint_t ToneDeviation;
          uint_t NoiseFreq;
        };

        Sample() : Loop()
        {
        }

        uint_t Loop;
        std::vector<Line> Lines;
      };

      struct Ornament
      {
        Ornament()
        {
        }

        uint_t Loop;
        std::vector<uint_t> Lines;
      };

      struct PositionEntry
      {
        PositionEntry() : PatternIndex(), Transposition()
        {
        }

        uint_t PatternIndex;
        uint_t Transposition;
      };

      class Builder
      {
      public:
        virtual ~Builder() {}

        virtual MetaBuilder& GetMetaBuilder() = 0;
        //common properties
        virtual void SetInitialTempo(uint_t tempo) = 0;
        //samples+ornaments
        virtual void SetSample(uint_t index, const Sample& sample) = 0;
        virtual void SetOrnament(uint_t index, const Ornament& ornament) = 0;
        //patterns
        virtual void SetPositions(const std::vector<PositionEntry>& positions, uint_t loop) = 0;

        virtual PatternBuilder& StartPattern(uint_t index) = 0;

        virtual void StartChannel(uint_t index) = 0;
        virtual void SetRest() = 0;
        virtual void SetNote(uint_t note) = 0;
        virtual void SetSample(uint_t sample) = 0;
        virtual void SetOrnament(uint_t ornament) = 0;
        virtual void SetAttenuation(uint_t vol) = 0;
        virtual void SetSwapSampleChannels(bool swapChannels) = 0;
        virtual void SetEnvelope(uint_t value) = 0;
        virtual void SetNoise(uint_t type) = 0;
      };

      Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
      Builder& GetStubBuilder();
    }
  }
}

#endif //FORMATS_CHIPTUNE_ETRACKER_H_DEFINED
