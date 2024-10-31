/**
 *
 * @file
 *
 * @brief  ExtremeTracker 1.xx support interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "formats/chiptune/builder_meta.h"
#include "formats/chiptune/builder_pattern.h"
#include "formats/chiptune/objects.h"

#include <formats/chiptune.h>

namespace Formats::Chiptune
{
  namespace ExtremeTracker1
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
      virtual void SetSamplesFrequency(uint_t freq) = 0;
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
      virtual void SetGliss(int_t gliss) = 0;
    };

    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
    Builder& GetStubBuilder();
  }  // namespace ExtremeTracker1

  Decoder::Ptr CreateExtremeTracker1Decoder();
}  // namespace Formats::Chiptune
