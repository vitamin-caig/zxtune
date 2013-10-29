/**
* 
* @file
*
* @brief  SoundTracker support interface
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
    namespace SoundTracker
    {
      struct Sample
      {
        struct Line
        {
          Line() : Level(), Noise(), NoiseMask(true), EnvelopeMask(true), Effect()
          {
          }

          uint_t Level;//0-15
          uint_t Noise;//0-31
          bool NoiseMask;
          bool EnvelopeMask;
          int_t Effect;
        };

        Sample() : Loop(), LoopLimit()
        {
        }

        uint_t Loop;
        uint_t LoopLimit;
        std::vector<Line> Lines;
      };

      typedef std::vector<int_t> Ornament;

      struct PositionEntry
      {
        PositionEntry() : PatternIndex(), Transposition()
        {
        }

        uint_t PatternIndex;
        int_t Transposition;
      };

      class Builder
      {
      public:
        virtual ~Builder() {}

        virtual MetaBuilder& GetMetaBuilder() = 0;
        virtual void SetInitialTempo(uint_t tempo) = 0;
        //samples+ornaments
        virtual void SetSample(uint_t index, const Sample& sample) = 0;
        virtual void SetOrnament(uint_t index, const Ornament& ornament) = 0;
        //patterns
        virtual void SetPositions(const std::vector<PositionEntry>& positions) = 0;

        virtual PatternBuilder& StartPattern(uint_t index) = 0;

        //! @invariant Channels are built sequentally
        virtual void StartChannel(uint_t index) = 0;
        virtual void SetRest() = 0;
        virtual void SetNote(uint_t note) = 0;
        virtual void SetSample(uint_t sample) = 0;
        virtual void SetOrnament(uint_t ornament) = 0;
        virtual void SetEnvelope(uint_t type, uint_t value) = 0;
        virtual void SetNoEnvelope() = 0;
      };

      Builder& GetStubBuilder();

      class Decoder : public Formats::Chiptune::Decoder
      {
      public:
        typedef boost::shared_ptr<const Decoder> Ptr;

        virtual Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target) const = 0;
      };

      namespace Ver1
      {
        Decoder::Ptr CreateUncompiledDecoder();
        Decoder::Ptr CreateCompiledDecoder();
      }

      namespace Ver3
      {
        Decoder::Ptr CreateDecoder();

        Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
        Binary::Container::Ptr InsertMetainformation(const Binary::Container& rawData, const Dump& info);
      }
    }
  }
}
