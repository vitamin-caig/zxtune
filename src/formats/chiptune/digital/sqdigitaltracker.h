/**
 *
 * @file
 *
 * @brief  SQDigitalTracker support interface
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
  namespace SQDigitalTracker
  {
    using Positions = LinesObject<uint_t>;

    class ChannelBuilder : public Formats::Chiptune::ChannelBuilder
    {
    public:
      using Formats::Chiptune::ChannelBuilder::SetVolume;

      virtual void SetVolume(uint_t volume, int_t slideDirection) = 0;
      virtual void SetVolumeSlidePeriod(uint_t period) = 0;
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
  }  // namespace SQDigitalTracker

  Decoder::Ptr CreateSQDigitalTrackerDecoder();
}  // namespace Formats::Chiptune
