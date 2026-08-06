/**
 *
 * @file
 *
 * @brief  Abyss' Highest Experience support interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "formats/chiptune/builder_meta.h"
#include "formats/chiptune/builder_pattern.h"

#include "formats/chiptune.h"

namespace Formats::Chiptune
{
  namespace AbyssHighestExperience
  {
    // Use simplified parsing due to thirdparty library used
    class Builder
    {
    public:
      virtual ~Builder() = default;

      virtual MetaBuilder& GetMetaBuilder() = 0;
      virtual void SetChannels(uint_t count) = 0;
    };

    Builder& GetStubBuilder();

    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);

    Decoder::Ptr CreateDecoder();

    namespace HivelyTracker
    {
      Decoder::Ptr CreateDecoder();

      Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
    }  // namespace HivelyTracker

    using Parser = decltype(&Parse);
  }  // namespace AbyssHighestExperience

  Decoder::Ptr CreateAbyssHighestExperienceDecoder();
  Decoder::Ptr CreateHivelyTrackerDecoder();
}  // namespace Formats::Chiptune
