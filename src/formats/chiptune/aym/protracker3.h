/**
* 
* @file
*
* @brief  ProTracker ve.x support interface
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
    namespace ProTracker3
    {
      const uint_t DEFAULT_SAMPLE = 1;
      const uint_t DEFAULT_ORNAMENT = 0;

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

        Sample(const Sample&) = delete;
        Sample& operator = (const Sample&) = delete;
        Sample(Sample&&) = default;
        Sample& operator = (Sample&&) = default;

        uint_t Loop;
        std::vector<Line> Lines;
      };

      struct Ornament
      {
        Ornament() : Loop()
        {
        }

        Ornament(const Ornament&) = delete;
        Ornament& operator = (const Ornament&) = delete;
        Ornament(Ornament&&) = default;
        Ornament& operator = (Ornament&&) = default;

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
        virtual ~Builder() = default;

        virtual MetaBuilder& GetMetaBuilder() = 0;
        //minor version number
        virtual void SetVersion(uint_t version) = 0;
        //version is set before this
        virtual void SetNoteTable(NoteTable table) = 0;
        virtual void SetMode(uint_t mode) = 0;
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

      Builder& GetStubBuilder();

      class Decoder : public Formats::Chiptune::Decoder
      {
      public:
        typedef std::shared_ptr<const Decoder> Ptr;

        virtual Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target) const = 0;
      };

      class ChiptuneBuilder : public Builder
      {
      public:
        typedef std::shared_ptr<ChiptuneBuilder> Ptr;
        virtual Binary::Data::Ptr GetResult() const = 0;
      };

      Decoder::Ptr CreateDecoder();
      Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);

      namespace VortexTracker2
      {
        Decoder::Ptr CreateDecoder();
        ChiptuneBuilder::Ptr CreateBuilder();
      }
    }

    Decoder::Ptr CreateProTracker3Decoder();
    Decoder::Ptr CreateVortexTracker2Decoder();
  }
}
