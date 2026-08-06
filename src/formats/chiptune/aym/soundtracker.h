/**
 *
 * @file
 *
 * @brief  SoundTracker support interface
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
  namespace SoundTracker
  {
    struct SampleLine
    {
      SampleLine() = default;

      uint_t Level = 0;  // 0-15
      uint_t Noise = 0;  // 0-31
      bool NoiseMask = true;
      bool EnvelopeMask = true;
      int_t Effect = 0;
    };

    using Sample = LinesObjectWithLoopLimit<SampleLine>;
    using Ornament = LinesObject<int_t>;

    struct PositionEntry
    {
      PositionEntry() = default;

      uint_t PatternIndex = 0;
      int_t Transposition = 0;
    };

    using Positions = LinesObject<PositionEntry>;

    class Builder
    {
    public:
      virtual ~Builder() = default;

      virtual MetaBuilder& GetMetaBuilder() = 0;
      virtual void SetInitialTempo(uint_t tempo) = 0;
      // samples+ornaments
      virtual void SetSample(uint_t index, Sample sample) = 0;
      virtual void SetOrnament(uint_t index, Ornament ornament) = 0;
      // patterns
      virtual void SetPositions(Positions positions) = 0;

      virtual PatternBuilder& StartPattern(uint_t index) = 0;

      //! @invariant Channels are built sequentally
      virtual void StartChannel(uint_t index) = 0;
      virtual void SetRest() = 0;
      virtual void SetNote(uint_t note) = 0;
      virtual void SetSample(uint_t sample) = 0;
      virtual void SetOrnament(uint_t ornament) = 0;
      virtual void SetEnvelope(uint_t type, uint_t value) = 0;
      virtual void SetNoEnvelope() = 0;
    };

    Builder& GetStubBuilder();

    namespace Ver1
    {
      Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
      Decoder::Ptr CreateDecoder();

      Formats::Chiptune::Container::Ptr ParseCompiled(const Binary::Container& data, Builder& target);
      Decoder::Ptr CreateCompiledDecoder();
    }  // namespace Ver1

    namespace Ver3
    {
      Decoder::Ptr CreateDecoder();

      Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
      Binary::Container::Ptr InsertMetainformation(const Binary::Container& rawData, Binary::View info);
    }  // namespace Ver3

    using Parser = decltype(&Ver1::Parse);
  }  // namespace SoundTracker

  Decoder::Ptr CreateSoundTrackerDecoder();
  Decoder::Ptr CreateSoundTrackerCompiledDecoder();
  Decoder::Ptr CreateSoundTracker3Decoder();
}  // namespace Formats::Chiptune
