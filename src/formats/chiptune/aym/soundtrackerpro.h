/**
 *
 * @file
 *
 * @brief  SoundTrackerPro support interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// local includes
#include "formats/chiptune/builder_meta.h"
#include "formats/chiptune/builder_pattern.h"
#include "formats/chiptune/objects.h"
// library includes
#include <formats/chiptune.h>

namespace Formats::Chiptune
{
  namespace SoundTrackerPro
  {
    const uint_t DEFAULT_SAMPLE = 0;
    const uint_t DEFAULT_ORNAMENT = 0;

    struct SampleLine
    {
      SampleLine() {}

      uint_t Level = 0;  // 0-15
      uint_t Noise = 0;  // 0-31
      bool ToneMask = true;
      bool NoiseMask = true;
      bool EnvelopeMask = true;
      int_t Vibrato = 0;
    };

    typedef LinesObject<SampleLine> Sample;
    typedef LinesObject<int_t> Ornament;

    struct PositionEntry
    {
      PositionEntry() {}

      uint_t PatternIndex = 0;
      int_t Transposition = 0;
    };

    typedef LinesObject<PositionEntry> Positions;

    class Builder
    {
    public:
      virtual ~Builder() = default;

      virtual MetaBuilder& GetMetaBuilder() = 0;
      // common properties
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
      virtual void SetEnvelope(uint_t type, uint_t value) = 0;
      virtual void SetNoEnvelope() = 0;
      virtual void SetGliss(uint_t target) = 0;
      virtual void SetVolume(uint_t vol) = 0;
    };

    Builder& GetStubBuilder();

    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      typedef std::shared_ptr<const Decoder> Ptr;

      virtual Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target) const = 0;
    };

    Decoder::Ptr CreateCompiledModulesDecoder();

    Formats::Chiptune::Container::Ptr ParseCompiled(const Binary::Container& data, Builder& target);
    Binary::Container::Ptr InsertMetaInformation(const Binary::Container& data, Binary::View info);
  }  // namespace SoundTrackerPro

  Decoder::Ptr CreateSoundTrackerProCompiledDecoder();
}  // namespace Formats::Chiptune
