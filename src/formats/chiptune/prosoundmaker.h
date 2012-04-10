/*
Abstract:
  ProSounMaker format description

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef FORMATS_CHIPTUNE_PROSOUNDMAKER_H_DEFINED
#define FORMATS_CHIPTUNE_PROSOUNDMAKER_H_DEFINED

//common includes
#include <binary/container.h>
//library includes
#include <formats/chiptune.h>
//boost includes
#include <boost/optional.hpp>

namespace Formats
{
  namespace Chiptune
  {
    namespace ProSoundMaker
    {
      struct Sample
      {
        struct Line
        {
          Line() : Level(), Noise(), ToneMask(), NoiseMask(), Gliss()
          {
          }

          uint_t Level;//0-15
          uint_t Noise;//0-31
          bool ToneMask;
          bool NoiseMask;
          int_t Gliss;
        };

        Sample() : VolumeDeltaPeriod(), VolumeDeltaValue()
        {
        }

        boost::optional<uint_t> Loop;
        uint_t VolumeDeltaPeriod;
        int_t VolumeDeltaValue;
        std::vector<Line> Lines;
      };

      struct Ornament
      {
        Ornament()
        {
        }

        boost::optional<uint_t> Loop;
        std::vector<int_t> Lines;
      };

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

        //common properties
        virtual void SetProgram(const String& program) = 0;
        virtual void SetTitle(const String& title) = 0;
        //samples+ornaments
        virtual void SetSample(uint_t index, const Sample& sample) = 0;
        virtual void SetOrnament(uint_t index, const Ornament& ornament) = 0;
        //patterns
        virtual void SetPositions(const std::vector<PositionEntry>& positions, uint_t loop) = 0;
        //building pattern -> line -> channel
        //! @invariant Patterns are built sequentally
        virtual void StartPattern(uint_t index) = 0;
        virtual void SetTempo(uint_t tempo) = 0;
        virtual void FinishPattern(uint_t size) = 0;
        //! @invariant Lines are built sequentally
        virtual void StartLine(uint_t index) = 0;
        //! @invariant Channels are built sequentally
        virtual void StartChannel(uint_t index) = 0;
        virtual void SetRest() = 0;
        virtual void SetNote(uint_t note) = 0;
        virtual void SetSample(uint_t sample) = 0;
        virtual void SetOrnament(uint_t ornament) = 0;
        virtual void SetVolume(uint_t volume) = 0;
        virtual void DisableOrnament() = 0;
        virtual void SetEnvelopeType(uint_t type) = 0;
        virtual void SetEnvelopeTone(uint_t tone) = 0;
        virtual void SetEnvelopeNote(uint_t note) = 0;
      };

      Formats::Chiptune::Container::Ptr ParseCompiled(const Binary::Container& data, Builder& target);
      Builder& GetStubBuilder();
    }
  }
}

#endif //FORMATS_CHIPTUNE_PROSOUNDMAKER_H_DEFINED
