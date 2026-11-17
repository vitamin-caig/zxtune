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

#include "formats/chiptune/common/builder_meta.h"
#include "formats/chiptune/common/builder_track.h"
#include "formats/chiptune/common/objects.h"

#include "binary/view.h"

namespace Formats::Chiptune::Digital
{
  using Positions = LinesObject<uint_t>;

  class Builder : public TrackBuilder
  {
  public:
    using TrackBuilder::SetSample;

    virtual ~Builder() = default;

    virtual MetaBuilder& GetMetaBuilder() = 0;
    // common properties
    virtual void SetInitialTempo(uint_t tempo) = 0;
    // samples
    virtual void SetSamplesFrequency(uint_t freq) = 0;
    virtual void SetSample(uint_t index, std::size_t loop, Binary::View sample, bool is4Bit) = 0;
    // patterns
    virtual void SetPositions(Positions positions) = 0;
  };

  Builder& GetStubBuilder();
}  // namespace Formats::Chiptune::Digital
