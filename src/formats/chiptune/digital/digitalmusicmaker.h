/**
 *
 * @file
 *
 * @brief  DigitalMusicMaker support interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "formats/chiptune/common/builder_meta.h"
#include "formats/chiptune/common/builder_pattern.h"
#include "formats/chiptune/common/objects.h"

#include "formats/chiptune/decoder.h"

namespace Formats::Chiptune
{
  namespace DigitalMusicMaker
  {
    using Positions = LinesObject<uint_t>;

    class ChannelBuilder : public Formats::Chiptune::ChannelBuilder
    {
    public:
      virtual void SetOrnament(uint_t ornament) = 0;
      virtual void SetTempo(uint_t tempo) = 0;

      virtual void SetFloat(int_t direction) = 0;
      virtual void SetFloatParam(uint_t step) = 0;
      virtual void SetVibrato() = 0;
      virtual void SetVibratoParams(uint_t step, uint_t period) = 0;
      virtual void SetArpeggio() = 0;
      virtual void SetArpeggioParams(uint_t step, uint_t period) = 0;
      virtual void SetSlide(int_t direction) = 0;
      virtual void SetSlideParams(uint_t step, uint_t period) = 0;
      virtual void SetDoubleNote() = 0;
      virtual void SetDoubleNoteParam(uint_t period) = 0;
      virtual void SetVolumeAttack() = 0;
      virtual void SetVolumeAttackParams(uint_t limit, uint_t period) = 0;
      virtual void SetVolumeDecay() = 0;
      virtual void SetVolumeDecayParams(uint_t limit, uint_t period) = 0;
      virtual void SetMixSample(uint_t idx) = 0;
      virtual void SetNoEffects() = 0;
    };

    using LineBuilder = Tracking<ChannelBuilder>::LineBuilder;
    using PatternBuilder = Tracking<ChannelBuilder>::PatternBuilder;

    class Builder
    {
    public:
      virtual ~Builder() = default;

      virtual MetaBuilder& GetMetaBuilder() = 0;
      // common properties
      virtual void SetInitialTempo(uint_t tempo) = 0;
      // samples
      virtual void SetSample(uint_t index, std::size_t loop, Binary::View sample) = 0;
      virtual std::unique_ptr<ChannelBuilder> SetSampleMixin(uint_t index, uint_t period) = 0;
      // patterns
      virtual void SetPositions(Positions positions) = 0;

      virtual PatternBuilder& StartPattern(uint_t index) = 0;
    };

    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
    Builder& GetStubBuilder();
  }  // namespace DigitalMusicMaker

  Decoder::Ptr CreateDigitalMusicMakerDecoder();
}  // namespace Formats::Chiptune
