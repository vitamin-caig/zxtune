/**
 *
 * @file
 *
 * @brief  TurboSound container support interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "formats/chiptune.h"

#include "types.h"

#include <memory>

namespace Formats::Chiptune
{
  namespace TurboSound
  {
    class Builder
    {
    public:
      using Ptr = std::shared_ptr<Builder>;
      virtual ~Builder() = default;

      virtual void SetFirstSubmoduleLocation(std::size_t offset, std::size_t size) = 0;
      virtual void SetSecondSubmoduleLocation(std::size_t offset, std::size_t size) = 0;
    };

    Builder& GetStubBuilder();

    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      using Ptr = std::shared_ptr<const Decoder>;

      virtual Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target) const = 0;
    };

    Decoder::Ptr CreateDecoder();
  }  // namespace TurboSound

  Decoder::Ptr CreateTurboSoundDecoder();
}  // namespace Formats::Chiptune
