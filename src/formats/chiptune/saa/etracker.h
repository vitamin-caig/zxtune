/**
 *
 * @file
 *
 * @brief  ETracker support interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "formats/chiptune/objects.h"

#include "formats/chiptune.h"

#include "types.h"

namespace Formats::Chiptune
{
  class MetaBuilder;
  class PatternBuilder;

  namespace ETracker
  {
    struct SampleLine
    {
      SampleLine() = default;

      uint_t LeftLevel = 0;   // 0-15
      uint_t RightLevel = 0;  // 0-15
      bool ToneEnabled = false;
      bool NoiseEnabled = false;
      uint_t ToneDeviation = 0;
      uint_t NoiseFreq = 0;
    };

    using Sample = LinesObject<SampleLine>;

    using Ornament = LinesObject<uint_t>;

    struct PositionEntry
    {
      PositionEntry() = default;

      uint_t PatternIndex = 0;
      uint_t Transposition = 0;
    };

    using Positions = LinesObject<PositionEntry>;

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
      virtual void SetAttenuation(uint_t vol) = 0;
      virtual void SetSwapSampleChannels(bool swapChannels) = 0;
      virtual void SetEnvelope(uint_t value) = 0;
      virtual void SetNoise(uint_t type) = 0;
    };

    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
    Builder& GetStubBuilder();
  }  // namespace ETracker

  Decoder::Ptr CreateETrackerDecoder();
}  // namespace Formats::Chiptune
