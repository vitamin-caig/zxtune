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

#include "formats/chiptune/objects.h"

#include "binary/view.h"
#include "formats/chiptune.h"

#include "types.h"

namespace Formats::Chiptune
{
  class MetaBuilder;
  class PatternBuilder;

  namespace SQDigitalTracker
  {
    using Positions = LinesObject<uint_t>;

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
      //! @invariant Channels are built sequentally
      virtual void StartChannel(uint_t index) = 0;
      virtual void SetRest() = 0;
      virtual void SetNote(uint_t note) = 0;
      virtual void SetSample(uint_t sample) = 0;
      virtual void SetVolume(uint_t volume) = 0;
      virtual void SetVolumeSlidePeriod(uint_t period) = 0;
      virtual void SetVolumeSlideDirection(int_t direction) = 0;
    };

    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
    Builder& GetStubBuilder();
  }  // namespace SQDigitalTracker

  Decoder::Ptr CreateSQDigitalTrackerDecoder();
}  // namespace Formats::Chiptune
