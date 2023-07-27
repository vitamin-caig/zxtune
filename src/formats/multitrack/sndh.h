/**
 *
 * @file
 *
 * @brief  SNDH support interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "formats/chiptune/common/builder_meta.h"

#include "formats/multitrack/container.h"
#include "time/duration.h"

#include <vector>

namespace Formats::Multitrack::SNDH
{
  class Builder
  {
  public:
    virtual ~Builder() = default;

    virtual Chiptune::MetaBuilder& GetMetaBuilder() = 0;
    virtual void SetYear(StringView year) = 0;
    virtual void SetUnknownTag(StringView tag, StringView value) = 0;

    // type = ['A', 'B', 'C', 'D', 'V']
    virtual void SetTimer(char type, uint_t freq) = 0;
    virtual void SetSubtunes(std::vector<StringView> names) = 0;
    // TIME tag: sub tune durations in seconds
    virtual void SetDurations(std::vector<Time::Seconds> durations) = 0;
    // FRMS tag: sub tune durations in player ticks (0 means endless loop)
    virtual void SetFrames(std::vector<uint_t> frames) = 0;
    // FLAG tag: raw flags string without leading '~'
    virtual void SetFlags(StringView flags) = 0;
  };

  Container::Ptr Parse(const Binary::Container& rawData, Builder& target);

  Builder& GetStubBuilder();
}  // namespace Formats::Multitrack::SNDH
