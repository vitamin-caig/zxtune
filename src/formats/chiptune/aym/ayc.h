/**
 *
 * @file
 *
 * @brief  AYC support interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "binary/view.h"
#include "formats/chiptune.h"

#include "types.h"

#include <memory>

namespace Formats::Chiptune
{
  namespace AYC
  {
    class Builder
    {
    public:
      using Ptr = std::shared_ptr<Builder>;
      virtual ~Builder() = default;

      virtual void SetFrames(std::size_t count) = 0;
      virtual void StartChannel(uint_t idx) = 0;
      virtual void AddValues(Binary::View values) = 0;
    };

    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
    Builder& GetStubBuilder();
  }  // namespace AYC

  Decoder::Ptr CreateAYCDecoder();
}  // namespace Formats::Chiptune
