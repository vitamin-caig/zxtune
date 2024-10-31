/**
 *
 * @file
 *
 * @brief  GlobalTracker support interface
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
  namespace GlobalTracker
  {
    struct SampleLine
    {
      SampleLine() = default;

      uint_t Level = 0;  // 0-15
      uint_t Noise = 0;  // 0-31
      bool ToneMask = true;
      bool NoiseMask = true;
      bool EnvelopeMask = true;
      uint_t Vibrato = 0;
    };

    using Sample = LinesObject<SampleLine>;

    using Ornament = LinesObject<int_t>;

    using Positions = LinesObject<uint_t>;

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
      virtual void SetVolume(uint_t vol) = 0;
    };

    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
    Builder& GetStubBuilder();
  }  // namespace GlobalTracker

  Decoder::Ptr CreateGlobalTrackerDecoder();
}  // namespace Formats::Chiptune
