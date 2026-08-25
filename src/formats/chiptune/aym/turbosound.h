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

#include "formats/chiptune/decoder.h"

#include "types.h"

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

    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);

    Decoder::Ptr CreateDecoder();

    using Parser = decltype(&Parse);
  }  // namespace TurboSound

  Decoder::Ptr CreateTurboSoundDecoder();
}  // namespace Formats::Chiptune
