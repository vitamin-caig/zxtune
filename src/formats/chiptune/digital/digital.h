/**
 *
 * @file
 *
 * @brief  Simple digital trackers support interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "formats/chiptune/objects.h"

#include "binary/view.h"

#include "types.h"

namespace Formats::Chiptune
{
  class MetaBuilder;
  class PatternBuilder;
}  // namespace Formats::Chiptune

namespace Formats::Chiptune::Digital
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
    virtual void SetSample(uint_t index, std::size_t loop, Binary::View sample, bool is4Bit) = 0;
    // patterns
    virtual void SetPositions(Positions positions) = 0;

    virtual PatternBuilder& StartPattern(uint_t index) = 0;

    //! @invariant Channels are built sequentally
    virtual void StartChannel(uint_t index) = 0;
    virtual void SetRest() = 0;
    virtual void SetNote(uint_t note) = 0;
    virtual void SetSample(uint_t sample) = 0;
  };

  Builder& GetStubBuilder();
}  // namespace Formats::Chiptune::Digital
