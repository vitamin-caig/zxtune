/**
 *
 * @file
 *
 * @brief  V2M parser interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "formats/chiptune/common/builder_meta.h"

#include "formats/chiptune/container.h"
#include "time/duration.h"

namespace Formats::Chiptune::V2m
{
  // Use simplified parsing due to thirdparty library used
  class Builder
  {
  public:
    virtual ~Builder() = default;

    virtual MetaBuilder& GetMetaBuilder() = 0;

    virtual void SetTotalDuration(Time::Milliseconds duration) = 0;
  };

  Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
  Builder& GetStubBuilder();
}  // namespace Formats::Chiptune::V2m
