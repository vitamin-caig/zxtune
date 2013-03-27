/*
Abstract:
  Pro Sound Creator format description

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef FORMATS_CHIPTUNE_PROSOUNDCREATOR_H_DEFINED
#define FORMATS_CHIPTUNE_PROSOUNDCREATOR_H_DEFINED

//common includes
#include <binary/container.h>
//library includes
#include <formats/chiptune.h>

namespace Formats
{
  namespace Chiptune
  {
    namespace ProSoundCreator
    {
      struct Sample
      {
        struct Line
        {
          Line()
            : Level(), Tone(), ToneMask(true), NoiseMask(true), Adding()
            , EnableEnvelope(), VolumeDelta()
          {
          }

          uint_t Level;//0-15
          uint_t Tone;
          bool ToneMask;
          bool NoiseMask;
          int_t Adding;
          bool EnableEnvelope;
          int_t VolumeDelta;//0/+1/-1
        };

        Sample()
          : Loop(), LoopLimit()
        {
        }

        uint_t Loop;
        uint_t LoopLimit;
        std::vector<Line> Lines;
      };

      struct Ornament
      {
        struct Line
        {
          Line()
            : NoteAddon(), NoiseAddon()
          {
          }
          int_t NoteAddon;
          int_t NoiseAddon;
        };

        Ornament()
          : Loop(), LoopLimit()
        {
        }

        uint_t Loop;
        uint_t LoopLimit;
        std::vector<Line> Lines;
      };

      class Builder
      {
      public:
        virtual ~Builder() {}

        //common properties
        virtual void SetProgram(const String& program) = 0;
        virtual void SetTitle(const String& title) = 0;
        virtual void SetAuthor(const String& author) = 0;
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
        //cmds
        virtual void SetEnvelope(uint_t type, uint_t tone) = 0;
        virtual void SetEnvelope() = 0;
        virtual void SetNoEnvelope() = 0;
        virtual void SetNoiseBase(uint_t val) = 0;
        virtual void SetBreakSample() = 0;
        virtual void SetBreakOrnament() = 0;
        virtual void SetNoOrnament() = 0;
        virtual void SetGliss(uint_t val) = 0;
        virtual void SetSlide(int_t steps) = 0;
        virtual void SetVolumeCounter(uint_t steps) = 0;
      };

      Builder& GetStubBuilder();
      Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
    }
  }
}

#endif //FORMATS_CHIPTUNE_PROSOUNDCREATOR_H_DEFINED
