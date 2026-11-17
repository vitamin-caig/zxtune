/**
 *
 * @file
 *
 * @brief  ChipTracker support interface
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
  namespace ChipTracker
  {
    using Positions = LinesObject<uint_t>;

    class ChannelBuilder : public Formats::Chiptune::ChannelBuilder
    {
    public:
      virtual void SetSlide(int_t step) = 0;
      virtual void SetSampleOffset(uint_t offset) = 0;
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
      // patterns
      virtual void SetPositions(Positions positions) = 0;
      virtual PatternBuilder& StartPattern(uint_t index) = 0;
    };

    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
    Builder& GetStubBuilder();
  }  // namespace ChipTracker

  Decoder::Ptr CreateChipTrackerDecoder();
}  // namespace Formats::Chiptune
