/**
* 
* @file
*
* @brief  ProTracker v1.x support interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "formats/chiptune/builder_meta.h"
#include "formats/chiptune/builder_pattern.h"
//library includes
#include <formats/chiptune.h>

namespace Formats
{
  namespace Chiptune
  {
    namespace ProTracker1
    {
      struct Sample
      {
        struct Line
        {
          Line() : Level(), Noise(), ToneMask(true), NoiseMask(true), Vibrato()
          {
          }

          uint_t Level;//0-15
          uint_t Noise;//0-31
          bool ToneMask;
          bool NoiseMask;
          int_t Vibrato;
        };

        Sample() : Loop()
        {
        }

        Sample(const Sample&) = delete;
        Sample& operator = (const Sample&) = delete;
        Sample(Sample&&) = default;
        Sample& operator = (Sample&&) = default;

        uint_t Loop;
        std::vector<Line> Lines;
      };

      struct Ornament
      {
        Ornament() = default;
        Ornament(const Ornament&) = delete;
        Ornament& operator = (const Ornament&) = delete;
        Ornament(Ornament&&) = default;
        Ornament& operator = (Ornament&&) = default;

        std::vector<int_t> Lines;
      };

      class Builder
      {
      public:
        virtual ~Builder() = default;

        virtual MetaBuilder& GetMetaBuilder() = 0;
        //common properties
        virtual void SetInitialTempo(uint_t tempo) = 0;
        //samples+ornaments
        virtual void SetSample(uint_t index, Sample sample) = 0;
        virtual void SetOrnament(uint_t index, Ornament ornament) = 0;
        //patterns
        virtual void SetPositions(std::vector<uint_t> positions, uint_t loop) = 0;

        virtual PatternBuilder& StartPattern(uint_t index) = 0;

        virtual void StartChannel(uint_t index) = 0;
        virtual void SetRest() = 0;
        virtual void SetNote(uint_t note) = 0;
        virtual void SetSample(uint_t sample) = 0;
        virtual void SetOrnament(uint_t ornament) = 0;
        virtual void SetVolume(uint_t vol) = 0;
        virtual void SetEnvelope(uint_t type, uint_t value) = 0;
        virtual void SetNoEnvelope() = 0;
      };

      Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
      Builder& GetStubBuilder();
    }

    Decoder::Ptr CreateProTracker1Decoder();
  }
}
