/*
Abstract:
  SoundTracker format description

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef FORMATS_CHIPTUNE_SOUNDTRACKER_H_DEFINED
#define FORMATS_CHIPTUNE_SOUNDTRACKER_H_DEFINED

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

      Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
      Formats::Chiptune::Container::Ptr ParseCompiled(const Binary::Container& data, Builder& target);
      //TODO: group together
      Formats::Chiptune::Container::Ptr ParseVersion3(const Binary::Container& rawData, Builder& target);
      Binary::Container::Ptr InsertVersion3Metainformation(const Binary::Container& rawData, const Dump& info);

      Builder& GetStubBuilder();
    }
  }
}

#endif //FORMATS_CHIPTUNE_SOUNDTRACKER_H_DEFINED
