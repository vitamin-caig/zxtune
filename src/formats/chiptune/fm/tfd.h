/**
 *
 * @file
 *
 * @brief  TurboFM Dump support interface
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
  class MetaBuilder;

  namespace TFD
  {
    class Builder
    {
    public:
      using Ptr = std::shared_ptr<Builder>;
      virtual ~Builder() = default;

      virtual MetaBuilder& GetMetaBuilder() = 0;

      virtual void BeginFrames(uint_t count) = 0;
      virtual void SelectChip(uint_t idx) = 0;
      virtual void SetLoop() = 0;
      virtual void SetRegister(uint_t idx, uint_t val) = 0;
    };

    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
    Builder& GetStubBuilder();
  }  // namespace TFD

  Decoder::Ptr CreateTFDDecoder();
}  // namespace Formats::Chiptune
