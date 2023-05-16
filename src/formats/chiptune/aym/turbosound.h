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

// common includes
#include <types.h>
// library includes
#include <formats/chiptune.h>

namespace Formats::Chiptune
{
  namespace TurboSound
  {
    class Builder
    {
    public:
      typedef std::shared_ptr<Builder> Ptr;
      virtual ~Builder() = default;

      virtual void SetFirstSubmoduleLocation(std::size_t offset, std::size_t size) = 0;
      virtual void SetSecondSubmoduleLocation(std::size_t offset, std::size_t size) = 0;
    };

    Builder& GetStubBuilder();

    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      typedef std::shared_ptr<const Decoder> Ptr;

      virtual Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target) const = 0;
    };

    Decoder::Ptr CreateDecoder();
  }  // namespace TurboSound

  Decoder::Ptr CreateTurboSoundDecoder();
}  // namespace Formats::Chiptune
