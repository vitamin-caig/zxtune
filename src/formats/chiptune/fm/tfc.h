/**
 *
 * @file
 *
 * @brief  TurboFM Compiled support interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "formats/chiptune/builder_meta.h"

#include <formats/chiptune.h>

#include <string_view.h>
#include <types.h>

namespace Formats::Chiptune
{
  namespace TFC
  {
    class Builder
    {
    public:
      using Ptr = std::shared_ptr<Builder>;
      virtual ~Builder() = default;

      virtual MetaBuilder& GetMetaBuilder() = 0;

      virtual void SetVersion(StringView version) = 0;
      virtual void SetIntFreq(uint_t freq) = 0;

      //! building channel->frame
      virtual void StartChannel(uint_t idx) = 0;
      virtual void StartFrame() = 0;
      virtual void SetSkip(uint_t count) = 0;
      virtual void SetLoop() = 0;
      virtual void SetSlide(uint_t slide) = 0;
      virtual void SetKeyOff() = 0;
      virtual void SetFreq(uint_t freq) = 0;
      virtual void SetRegister(uint_t reg, uint_t val) = 0;
      virtual void SetKeyOn() = 0;
    };

    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
    Builder& GetStubBuilder();
  }  // namespace TFC

  Decoder::Ptr CreateTFCDecoder();
}  // namespace Formats::Chiptune
