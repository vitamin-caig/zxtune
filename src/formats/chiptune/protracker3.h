/*
Abstract:
  ProTracker v3.xx format description

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef FORMATS_CHIPTUNE_PROTRACKER3_H_DEFINED
#define FORMATS_CHIPTUNE_PROTRACKER3_H_DEFINED

//common includes
#include <binary/container.h>
//library includes
#include <formats/chiptune.h>

namespace Formats
{
  namespace Chiptune
  {
    namespace ProTracker3
    {
      struct Sample
      {
        struct Line
        {
          Line()
           : Level(), VolumeSlideAddon()
           , ToneMask(true), ToneOffset(), KeepToneOffset()
           , NoiseMask(true), EnvMask(true), NoiseOrEnvelopeOffset(), KeepNoiseOrEnvelopeOffset()
          {
          }

          uint_t Level;//0-15
          int_t VolumeSlideAddon;
          bool ToneMask;
          int_t ToneOffset;
          bool KeepToneOffset;
          bool NoiseMask;
          bool EnvMask;
          int_t NoiseOrEnvelopeOffset;
          bool KeepNoiseOrEnvelopeOffset;
        };

        Sample() : Loop()
        {
        }

        uint_t Loop;
        std::vector<Line> Lines;
      };

      struct Ornament
      {
        Ornament() : Loop()
        {
        }

        uint_t Loop;
        std::vector<int_t> Lines;
      };

      enum NoteTable
      {
        PROTRACKER,
        SOUNDTRACKER,
        ASM,
        REAL,
        NATURAL
      };

      const uint_t SINGLE_AY_MODE = 0x20;

      class Builder
      {
      public:
        virtual ~Builder() {}

        //common properties
        virtual void SetProgram(const String& program) = 0;
        virtual void SetTitle(const String& title) = 0;
        virtual void SetAuthor(const String& author) = 0;
        //minor version number
        virtual void SetVersion(uint_t version) = 0;
        //version is set before this
        virtual void SetNoteTable(NoteTable table) = 0;
        virtual void SetMode(uint_t mode) = 0;
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
        virtual void SetGlissade(uint_t period, int_t val) = 0;
        virtual void SetNoteGliss(uint_t period, int_t val, uint_t limit) = 0;
        virtual void SetSampleOffset(uint_t offset) = 0;
        virtual void SetOrnamentOffset(uint_t offset) = 0;
        virtual void SetVibrate(uint_t ontime, uint_t offtime) = 0;
        virtual void SetEnvelopeSlide(uint_t period, int_t val) = 0;
        virtual void SetEnvelope(uint_t type, uint_t value) = 0;
        virtual void SetNoEnvelope() = 0;
        virtual void SetNoiseBase(uint_t val) = 0;
      };

      Formats::Chiptune::Container::Ptr ParseCompiled(const Binary::Container& data, Builder& target);
      Formats::Chiptune::Container::Ptr ParseVortexTracker2(const Binary::Container& data, Builder& target);
      Builder& GetStubBuilder();
    }
  }
}

#endif //FORMATS_CHIPTUNE_PROTRACKER3_H_DEFINED
