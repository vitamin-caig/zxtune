/**
 *
 * @file
 *
 * @brief  SPC support interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "binary/view.h"
#include "formats/chiptune.h"
#include "time/duration.h"

#include "string_view.h"
#include "types.h"

namespace Formats::Chiptune
{
  class MetaBuilder;

  namespace SPC
  {
    class Builder
    {
    public:
      virtual ~Builder() = default;

      virtual MetaBuilder& GetMetaBuilder() = 0;

      virtual void SetRegisters(uint16_t pc, uint8_t a, uint8_t x, uint8_t y, uint8_t psw, uint8_t sp) = 0;

      virtual void SetDumper(StringView dumper) = 0;
      virtual void SetDumpDate(StringView date) = 0;
      virtual void SetIntro(Time::Milliseconds duration) = 0;
      virtual void SetLoop(Time::Milliseconds duration) = 0;
      virtual void SetFade(Time::Milliseconds duration) = 0;

      virtual void SetRAM(Binary::View data) = 0;
      virtual void SetDSPRegisters(Binary::View data) = 0;
      virtual void SetExtraRAM(Binary::View data) = 0;
    };

    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
    Builder& GetStubBuilder();
  }  // namespace SPC

  Decoder::Ptr CreateSPCDecoder();
}  // namespace Formats::Chiptune
