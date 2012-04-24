/*
Abstract:
  ProTracker v1.xx format description

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef FORMATS_CHIPTUNE_PROTRACKER1_H_DEFINED
#define FORMATS_CHIPTUNE_PROTRACKER1_H_DEFINED

//common includes
#include <binary/container.h>
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

        uint_t Loop;
        std::vector<Line> Lines;
      };

      struct Ornament
      {
        Ornament()
        {
        }

        std::vector<int_t> Lines;
      };

      class Builder
      {
      public:
        virtual ~Builder() {}

        //common properties
        virtual void SetProgram(const String& program) = 0;
        virtual void SetTitle(const String& title) = 0;
        virtual void SetInitialTempo(uint_t tempo) = 0;
        //samples+ornaments
        virtual void SetSample(uint_t index, const Sample& sample) = 0;
        virtual void SetOrnament(uint_t index, const Ornament& ornament) = 0;
        //patterns
        virtual void SetPositions(const std::vector<uint_t>& positions, uint_t loop) = 0;
        //building pattern -> line -> channel
        //! @invariant Patterns are built sequentally
        virtual void StartPattern(uint_t index) = 0;
        virtual void FinishPattern(uint_t size) = 0;
        //! @invariant Lines are built sequentally
        virtual void StartLine(uint_t index) = 0;
        virtual void SetTempo(uint_t tempo) = 0;
        //! @invariant Channels are built sequentally
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
  }
}

#endif //FORMATS_CHIPTUNE_PROTRACKER1_H_DEFINED
