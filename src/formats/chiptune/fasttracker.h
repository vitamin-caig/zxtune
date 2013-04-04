/*
Abstract:
  FastTracker format description

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef FORMATS_CHIPTUNE_FASTTRACKER_H_DEFINED
#define FORMATS_CHIPTUNE_FASTTRACKER_H_DEFINED

//local includes
#include "builder_meta.h"
//common includes
#include <binary/container.h>
//library includes
#include <formats/chiptune.h>

namespace Formats
{
  namespace Chiptune
  {
    namespace FastTracker
    {
      struct Sample
      {
        struct Line
        {
          Line()
            : Level(), VolSlide()
            , Noise(), AccumulateNoise(), NoiseMask(true)
            , Tone(), AccumulateTone(), ToneMask(true)
            , EnvelopeAddon(), AccumulateEnvelope(), EnableEnvelope()
          {
          }

          uint_t Level;//0-15
          int_t VolSlide;//0/+1/-1
          uint_t Noise;
          bool AccumulateNoise;
          bool NoiseMask;
          uint_t Tone;
          bool AccumulateTone;
          bool ToneMask;
          uint_t EnvelopeAddon;
          bool AccumulateEnvelope;
          bool EnableEnvelope;
        };

        Sample()
          : Loop()
        {
        }

        uint_t Loop;
        std::vector<Line> Lines;
      };

      struct Ornament
      {
        struct Line
        {
          Line()
            : NoteAddon(), KeepNoteAddon(), NoiseAddon(), KeepNoiseAddon()
          {
          }

          int_t NoteAddon;
          bool KeepNoteAddon;
          int_t NoiseAddon;
          bool KeepNoiseAddon;
        };

        Ornament()
          : Loop()
        {
        }

        uint_t Loop;
        std::vector<Line> Lines;
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

        virtual MetaBuilder& GetMetaBuilder() = 0;
        //common properties
        virtual void SetInitialTempo(uint_t tempo) = 0;
        //samples+ornaments
        virtual void SetSample(uint_t index, const Sample& sample) = 0;
        virtual void SetOrnament(uint_t index, const Ornament& ornament) = 0;
        //patterns
        virtual void SetPositions(const std::vector<PositionEntry>& positions, uint_t loop) = 0;
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
        //cmds
        virtual void SetEnvelope(uint_t type, uint_t tone) = 0;
        virtual void SetNoEnvelope() = 0;
        virtual void SetNoise(uint_t val) = 0;
        virtual void SetSlide(uint_t steps) = 0;
        virtual void SetNoteSlide(uint_t steps) = 0;
      };

      Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
      Builder& GetStubBuilder();
    }
  }
}

#endif //FORMATS_CHIPTUNE_FASTTRACKER_H_DEFINED
