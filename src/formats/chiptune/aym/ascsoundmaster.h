/**
 *
 * @file
 *
 * @brief  ASC Sound Master support interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "formats/chiptune/objects.h"

#include "binary/view.h"
#include "formats/chiptune.h"

#include "types.h"

#include <memory>

namespace Formats::Chiptune
{
  class MetaBuilder;
  class PatternBuilder;

  namespace ASCSoundMaster
  {
    struct SampleLine
    {
      SampleLine() = default;

      uint_t Level = 0;  // 0-15
      int_t ToneDeviation = 0;
      bool ToneMask = true;
      bool NoiseMask = true;
      int_t Adding = 0;
      bool EnableEnvelope = false;
      int_t VolSlide = 0;  // 0/+1/-1
    };

    using Sample = LinesObjectWithLoopLimit<SampleLine>;

    struct OrnamentLine
    {
      OrnamentLine() = default;
      int_t NoteAddon = 0;
      int_t NoiseAddon = 0;
    };

    using Ornament = LinesObjectWithLoopLimit<OrnamentLine>;

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
      virtual void SetVolume(uint_t vol) = 0;
      virtual void SetEnvelopeType(uint_t type) = 0;
      virtual void SetEnvelopeTone(uint_t tone) = 0;
      virtual void SetEnvelope() = 0;
      virtual void SetNoEnvelope() = 0;
      virtual void SetNoise(uint_t val) = 0;
      virtual void SetContinueSample() = 0;
      virtual void SetContinueOrnament() = 0;
      virtual void SetGlissade(int_t val) = 0;
      virtual void SetSlide(int_t steps, bool useToneSliding) = 0;
      virtual void SetVolumeSlide(uint_t period, int_t delta) = 0;
      virtual void SetBreakSample() = 0;
    };

    Builder& GetStubBuilder();

    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      using Ptr = std::shared_ptr<const Decoder>;

      virtual Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target) const = 0;
    };

    namespace Ver0
    {
      Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
      Binary::Container::Ptr InsertMetaInformation(const Binary::Container& data, Binary::View info);

      Decoder::Ptr CreateDecoder();
    };  // namespace Ver0

    namespace Ver1
    {
      Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
      Binary::Container::Ptr InsertMetaInformation(const Binary::Container& data, Binary::View info);

      Decoder::Ptr CreateDecoder();
    };  // namespace Ver1
  }     // namespace ASCSoundMaster

  Decoder::Ptr CreateASCSoundMaster0xDecoder();
  Decoder::Ptr CreateASCSoundMaster1xDecoder();
}  // namespace Formats::Chiptune
