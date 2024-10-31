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

#include "formats/chiptune/builder_meta.h"
#include "formats/chiptune/builder_pattern.h"
#include "formats/chiptune/objects.h"

#include "formats/chiptune.h"

namespace Formats::Chiptune
{
  namespace ProTracker3
  {
    const uint_t DEFAULT_SAMPLE = 1;
    const uint_t DEFAULT_ORNAMENT = 0;

    struct SampleLine
    {
      SampleLine() = default;

      uint_t Level = 0;  // 0-15
      int_t VolumeSlideAddon = 0;
      bool ToneMask = true;
      int_t ToneOffset = 0;
      bool KeepToneOffset = false;
      bool NoiseMask = true;
      bool EnvMask = true;
      int_t NoiseOrEnvelopeOffset = 0;
      bool KeepNoiseOrEnvelopeOffset = false;
    };

    using Sample = LinesObject<SampleLine>;
    using Ornament = LinesObject<int_t>;
    using Positions = LinesObject<uint_t>;

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
      // minor version number
      virtual void SetVersion(uint_t version) = 0;
      // version is set before this
      virtual void SetNoteTable(NoteTable table) = 0;
      virtual void SetMode(uint_t mode) = 0;
      virtual void SetInitialTempo(uint_t tempo) = 0;
      // samples+ornaments
      virtual void SetSample(uint_t index, Sample sample) = 0;
      virtual void SetOrnament(uint_t index, Ornament ornament) = 0;
      // patterns
      virtual void SetPositions(Positions positions) = 0;

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
      using Ptr = std::shared_ptr<const Decoder>;

      virtual Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target) const = 0;
    };

    class ChiptuneBuilder : public Builder
    {
    public:
      using Ptr = std::shared_ptr<ChiptuneBuilder>;
      virtual Binary::Data::Ptr GetResult() const = 0;
    };

    Decoder::Ptr CreateDecoder();
    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);

    namespace VortexTracker2
    {
      Decoder::Ptr CreateDecoder();
      ChiptuneBuilder::Ptr CreateBuilder();
    }  // namespace VortexTracker2
  }    // namespace ProTracker3

  Decoder::Ptr CreateProTracker3Decoder();
  Decoder::Ptr CreateVortexTracker2Decoder();
}  // namespace Formats::Chiptune
