/*
Abstract:
  SQTracker format description

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef FORMATS_CHIPTUNE_SQTRACKER_H_DEFINED
#define FORMATS_CHIPTUNE_SQTRACKER_H_DEFINED

//common includes
#include <types.h>
//library includes
#include <binary/container.h>
#include <formats/chiptune.h>

namespace Formats
{
  namespace Chiptune
  {
    namespace SQTracker
    {
      struct Sample
      {
        struct Line
        {
          Line() : Level(), Noise(), ToneDeviation(), EnableNoise(), EnableTone()
          {
          }

          uint_t Level;//0-15
          uint_t Noise;//0-31
          int_t ToneDeviation;
          bool EnableNoise;
          bool EnableTone;
        };

        Sample() : Loop(), LoopSize()
        {
        }

        uint_t Loop;
        uint_t LoopSize;
        std::vector<Line> Lines;
      };

      struct Ornament
      {
        Ornament() : Loop(), LoopSize()
        {
        }

        uint_t Loop;
        uint_t LoopSize;
        std::vector<int_t> Lines;
      };

      struct PositionEntry
      {
        PositionEntry() : Tempo()
        {
        }

        struct Channel
        {
          Channel() : Pattern(), Transposition(), Attenuation(), EnabledEffects(true)
          {
          }

          uint_t Pattern;
          int_t Transposition;
          uint_t Attenuation;
          bool EnabledEffects;
        };

        uint_t Tempo;
        Channel Channels[3];
      };

      class Builder
      {
      public:
        virtual ~Builder() {}

        //common properties
        virtual void SetProgram(const String& program) = 0;
        //samples+ornaments
        virtual void SetSample(uint_t index, const Sample& sample) = 0;
        virtual void SetOrnament(uint_t index, const Ornament& ornament) = 0;
        //patterns
        virtual void SetPositions(const std::vector<PositionEntry>& positions, uint_t loop) = 0;
        //building pattern -> line
        //! @invariant Patterns are built sequentally
        virtual void StartPattern(uint_t index) = 0;
        virtual void FinishPattern(uint_t size) = 0;
        //! @invariant Lines are built sequentally
        virtual void StartLine(uint_t index) = 0;
        virtual void SetTempo(uint_t tempo) = 0;
        virtual void SetTempoAddon(uint_t add) = 0;
        virtual void SetRest() = 0;
        virtual void SetNote(uint_t note) = 0;
        virtual void SetNoteAddon(int_t add) = 0;
        virtual void SetSample(uint_t sample) = 0;
        virtual void SetOrnament(uint_t ornament) = 0;
        virtual void SetEnvelope(uint_t type, uint_t value) = 0;
        virtual void SetGlissade(int_t step) = 0;
        virtual void SetAttenuation(uint_t att) = 0;
        virtual void SetAttenuationAddon(int_t add) = 0;
        virtual void SetGlobalAttenuation(uint_t att) = 0;
        virtual void SetGlobalAttenuationAddon(int_t add) = 0;
      };

      Formats::Chiptune::Container::Ptr ParseCompiled(const Binary::Container& data, Builder& target);
      Builder& GetStubBuilder();
    }
  }
}

#endif //FORMATS_CHIPTUNE_SQTRACKER_H_DEFINED
