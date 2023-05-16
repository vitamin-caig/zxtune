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

// local includes
#include "formats/chiptune/builder_meta.h"
// common includes
#include <types.h>
// library includes
#include <formats/chiptune.h>

namespace Formats::Chiptune
{
  namespace TFC
  {
    class Builder
    {
    public:
      typedef std::shared_ptr<Builder> Ptr;
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
