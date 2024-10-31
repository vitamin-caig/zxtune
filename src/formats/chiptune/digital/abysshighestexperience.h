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

#include "formats/chiptune.h"

#include <memory>

namespace Formats::Chiptune
{
  class MetaBuilder;

  namespace AbyssHighestExperience
  {
    // Use simplified parsing due to thirdparty library used
    class Builder
    {
    public:
      virtual ~Builder() = default;

      virtual MetaBuilder& GetMetaBuilder() = 0;
    };

    Builder& GetStubBuilder();

    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      using Ptr = std::shared_ptr<const Decoder>;

      virtual Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target) const = 0;
    };

    Decoder::Ptr CreateDecoder();

    namespace HivelyTracker
    {
      Decoder::Ptr CreateDecoder();
    }
  }  // namespace AbyssHighestExperience

  Decoder::Ptr CreateAbyssHighestExperienceDecoder();
  Decoder::Ptr CreateHivelyTrackerDecoder();
}  // namespace Formats::Chiptune
