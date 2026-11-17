/**
 *
 * @file
 *
 * @brief  ProDigiTracker support interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "formats/chiptune/common/builder_meta.h"
#include "formats/chiptune/common/builder_track.h"
#include "formats/chiptune/common/objects.h"

#include "formats/chiptune/decoder.h"

namespace Formats::Chiptune
{
  namespace ProDigiTracker
  {
    using Ornament = LinesObject<int_t>;

    using Positions = LinesObject<uint_t>;

    class Builder : public TrackBuilder
    {
    public:
      using TrackBuilder::SetSample;
      using TrackBuilder::SetOrnament;

      virtual ~Builder() = default;

      virtual MetaBuilder& GetMetaBuilder() = 0;
      // common properties
      virtual void SetInitialTempo(uint_t tempo) = 0;
      // samples
      virtual void SetSample(uint_t index, std::size_t loop, Binary::View sample) = 0;
      virtual void SetOrnament(uint_t index, Ornament ornament) = 0;
      // patterns
      virtual void SetPositions(Positions positions) = 0;
    };

    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
    Builder& GetStubBuilder();
  }  // namespace ProDigiTracker

  Decoder::Ptr CreateProDigiTrackerDecoder();
}  // namespace Formats::Chiptune
