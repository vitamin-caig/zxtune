/*
Abstract:
  ASC Sound Master format description

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef FORMATS_CHIPTUNE_ASCSOUNDMASTER_H_DEFINED
#define FORMATS_CHIPTUNE_ASCSOUNDMASTER_H_DEFINED

//common includes
#include <binary/container.h>
//library includes
#include <formats/chiptune.h>

namespace Formats
{
  namespace Chiptune
  {
    namespace ASCSoundMaster
    {
      struct Sample
      {
        struct Line
        {
          Line()
            : Level(), ToneDeviation(), ToneMask(), NoiseMask(), Adding()
            , EnableEnvelope(), VolSlide()
          {
          }

          uint_t Level;//0-15
          int_t ToneDeviation;
          bool ToneMask;
          bool NoiseMask;
          int_t Adding;
          bool EnableEnvelope;
          int_t VolSlide;//0/+1/-1
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
        virtual void SetEnvelopeType(uint_t type) = 0;
        virtual void SetEnvelopeTone(uint_t tone) = 0;
        virtual void SetEnvelope() = 0;
        virtual void SetNoEnvelope() = 0;
        virtual void SetNoise(uint_t val) = 0;
        virtual void SetContinueSample() = 0;
        virtual void SetContinueOrnament() = 0;
        virtual void SetGlissade(int_t val) = 0;
        virtual void SetSlide(int_t steps) = 0;
        virtual void SetVolumeSlide(uint_t period, int_t delta) = 0;
        virtual void SetBreakSample() = 0;
      };

      Formats::Chiptune::Container::Ptr ParseVersion0x(const Binary::Container& data, Builder& target);
      Formats::Chiptune::Container::Ptr ParseVersion1x(const Binary::Container& data, Builder& target);
      Builder& GetStubBuilder();
    }
  }
}

#endif //FORMATS_CHIPTUNE_ASCSOUNDMASTER_H_DEFINED
