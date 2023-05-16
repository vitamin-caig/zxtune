/**
 *
 * @file
 *
 * @brief  SQTracker support interface
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
// std includes
#include <array>

namespace Formats::Chiptune
{
  namespace SQTracker
  {
    struct SampleLine
    {
      SampleLine()
        : Level()
        , Noise()
        , ToneDeviation()
        , EnableNoise()
        , EnableTone()
      {}

      uint_t Level;  // 0-15
      uint_t Noise;  // 0-31
      int_t ToneDeviation;
      bool EnableNoise;
      bool EnableTone;
    };

    typedef LinesObjectWithLoopLimit<SampleLine> Sample;

    typedef LinesObjectWithLoopLimit<int_t> Ornament;

    struct PositionEntry
    {
      PositionEntry()
        : Tempo()
      {}

      struct Channel
      {
        Channel()
          : Pattern()
          , Transposition()
          , Attenuation()
          , EnabledEffects(true)
        {}

        uint_t Pattern;
        int_t Transposition;
        uint_t Attenuation;
        bool EnabledEffects;
      };

      uint_t Tempo;
      std::array<Channel, 3> Channels;
    };

    typedef LinesObject<PositionEntry> Positions;

    class Builder
    {
    public:
      virtual ~Builder() = default;

      virtual MetaBuilder& GetMetaBuilder() = 0;
      // samples+ornaments
      virtual void SetSample(uint_t index, Sample sample) = 0;
      virtual void SetOrnament(uint_t index, Ornament ornament) = 0;
      // patterns
      virtual void SetPositions(Positions positions) = 0;

      virtual PatternBuilder& StartPattern(uint_t index) = 0;

      virtual void SetTempoAddon(uint_t add) = 0;
      virtual void SetRest() = 0;
      virtual void SetNote(uint_t note) = 0;
      virtual void SetSample(uint_t sample) = 0;
      virtual void SetOrnament(uint_t ornament) = 0;
      virtual void SetEnvelope(uint_t type, uint_t value) = 0;
      virtual void SetGlissade(int_t step) = 0;
      virtual void SetAttenuation(uint_t att) = 0;
      virtual void SetAttenuationAddon(int_t add) = 0;
      virtual void SetGlobalAttenuation(uint_t att) = 0;
      virtual void SetGlobalAttenuationAddon(int_t add) = 0;
    };

    Formats::Chiptune::Container::Ptr ParseCompiled(const Binary::Container& data, Builder& target);
    Builder& GetStubBuilder();
  }  // namespace SQTracker

  Decoder::Ptr CreateSQTrackerDecoder();
}  // namespace Formats::Chiptune
