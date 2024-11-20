/**
 *
 * @file
 *
 * @brief  YM/VTX support interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "formats/chiptune/builder_meta.h"

#include "binary/view.h"
#include "formats/chiptune.h"

#include "string_view.h"
#include "types.h"

namespace Formats::Chiptune
{
  namespace YM
  {
    class Builder
    {
    public:
      using Ptr = std::shared_ptr<Builder>;
      virtual ~Builder() = default;

      virtual MetaBuilder& GetMetaBuilder() = 0;
      // YMx
      virtual void SetVersion(StringView version) = 0;
      // Default: false
      virtual void SetChipType(bool ym) = 0;
      // Default: abc.
      // 0 - mono, 1- abc, 2- acb, 3- bac, 4- bca, 5- cab, 6- cba
      virtual void SetStereoMode(uint_t mode) = 0;
      // Default: 0
      virtual void SetLoop(uint_t loop) = 0;
      virtual void SetDigitalSample(uint_t idx, Binary::View data) = 0;
      virtual void SetClockrate(uint64_t freq) = 0;
      // Default: 50
      virtual void SetIntFreq(uint_t freq) = 0;
      virtual void SetYear(uint_t year) = 0;
      virtual void SetEditor(StringView editor) = 0;

      virtual void AddData(Binary::View registers) = 0;
    };

    Builder& GetStubBuilder();

    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      using Ptr = std::shared_ptr<const Decoder>;

      virtual Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target) const = 0;
    };

    Decoder::Ptr CreatePackedYMDecoder();
    Decoder::Ptr CreateYMDecoder();
    Decoder::Ptr CreateVTXDecoder();
  }  // namespace YM

  Decoder::Ptr CreatePackedYMDecoder();
  Decoder::Ptr CreateYMDecoder();
  Decoder::Ptr CreateVTXDecoder();
}  // namespace Formats::Chiptune
