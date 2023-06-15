/**
 *
 * @file
 *
 * @brief  PSG support interface
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
  namespace PSG
  {
    class Builder
    {
    public:
      using Ptr = std::shared_ptr<Builder>;
      virtual ~Builder() = default;

      virtual void AddChunks(std::size_t count) = 0;
      virtual void SetRegister(uint_t reg, uint_t val) = 0;
    };

    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
    Builder& GetStubBuilder();
  }  // namespace PSG

  Decoder::Ptr CreatePSGDecoder();
}  // namespace Formats::Chiptune
