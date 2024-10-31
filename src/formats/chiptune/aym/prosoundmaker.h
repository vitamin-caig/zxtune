/**
 *
 * @file
 *
 * @brief  ProSoundMaker support interface
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
  namespace ProSoundMaker
  {
    struct SampleLine
    {
      SampleLine() = default;

      uint_t Level = 0;  // 0-15
      uint_t Noise = 0;  // 0-31
      bool ToneMask = true;
      bool NoiseMask = true;
      int_t Gliss = 0;
    };

    class Sample : public LinesObject<SampleLine>
    {
    public:
      Sample() = default;

      Sample(const Sample&) = delete;
      Sample& operator=(const Sample&) = delete;

      Sample(Sample&& rh) noexcept = default;
      Sample& operator=(Sample&& rh) noexcept = default;

      uint_t VolumeDeltaPeriod = 0;
      uint_t VolumeDeltaValue = 0;
    };

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
      virtual void SetVolume(uint_t volume) = 0;
      virtual void DisableOrnament() = 0;
      virtual void SetEnvelopeReinit(bool enabled) = 0;
      virtual void SetEnvelopeTone(uint_t type, uint_t tone) = 0;
      virtual void SetEnvelopeNote(uint_t type, uint_t note) = 0;
    };

    Formats::Chiptune::Container::Ptr ParseCompiled(const Binary::Container& data, Builder& target);
    Builder& GetStubBuilder();
  }  // namespace ProSoundMaker

  Decoder::Ptr CreateProSoundMakerCompiledDecoder();
}  // namespace Formats::Chiptune
