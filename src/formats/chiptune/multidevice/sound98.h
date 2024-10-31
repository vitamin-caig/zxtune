/**
 *
 * @file
 *
 * @brief  S98 support interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "formats/chiptune/builder_meta.h"

#include <formats/chiptune.h>
#include <time/duration.h>

namespace Formats::Chiptune
{
  namespace Sound98
  {
    class Builder
    {
    public:
      virtual ~Builder() = default;

      virtual MetaBuilder& GetMetaBuilder() = 0;

      virtual void SetTimings(Time::Milliseconds total, Time::Milliseconds loop) = 0;
    };

    Builder& GetStubBuilder();

    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
  }  // namespace Sound98

  Decoder::Ptr CreateSound98Decoder();
}  // namespace Formats::Chiptune
