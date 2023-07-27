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

#include "formats/chiptune/builder_meta.h"

#include "formats/multitrack.h"
#include "time/duration.h"

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
    virtual void SetDurations(std::vector<Time::Seconds> durations) = 0;
  };

  Container::Ptr Parse(const Binary::Container& rawData, Builder& target);

  Builder& GetStubBuilder();
}  // namespace Formats::Multitrack::SNDH
