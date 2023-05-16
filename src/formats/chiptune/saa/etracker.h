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

// local includes
#include "formats/chiptune/builder_meta.h"
#include "formats/chiptune/builder_pattern.h"
#include "formats/chiptune/objects.h"
// library includes
#include <formats/chiptune.h>

namespace Formats::Chiptune
{
  namespace ETracker
  {
    struct SampleLine
    {
      SampleLine()
        : LeftLevel()
        , RightLevel()
        , ToneEnabled()
        , NoiseEnabled()
        , ToneDeviation()
        , NoiseFreq()
      {}

      uint_t LeftLevel;   // 0-15
      uint_t RightLevel;  // 0-15
      bool ToneEnabled;
      bool NoiseEnabled;
      uint_t ToneDeviation;
      uint_t NoiseFreq;
    };

    typedef LinesObject<SampleLine> Sample;

    typedef LinesObject<uint_t> Ornament;

    struct PositionEntry
    {
      PositionEntry()
        : PatternIndex()
        , Transposition()
      {}

      uint_t PatternIndex;
      uint_t Transposition;
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
