/**
 *
 * @file
 *
 * @brief  AY/EMUL support interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "formats/chiptune/builder_meta.h"

#include "binary/view.h"
#include "formats/chiptune.h"

#include "types.h"

namespace Formats::Chiptune
{
  namespace AY
  {
    class Builder
    {
    public:
      virtual ~Builder() = default;

      virtual MetaBuilder& GetMetaBuilder() = 0;

      virtual void SetDuration(uint_t total, uint_t fadeout) = 0;
      virtual void SetRegisters(uint16_t reg, uint16_t sp) = 0;
      virtual void SetRoutines(uint16_t init, uint16_t play) = 0;
      virtual void AddBlock(uint16_t addr, Binary::View data) = 0;
    };

    uint_t GetModulesCount(Binary::View data);
    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, std::size_t idx, Builder& target);
    Builder& GetStubBuilder();

    class BlobBuilder : public Builder
    {
    public:
      using Ptr = std::shared_ptr<BlobBuilder>;

      virtual Binary::Container::Ptr Result() const = 0;
    };

    BlobBuilder::Ptr CreateMemoryDumpBuilder();
    BlobBuilder::Ptr CreateFileBuilder();
  }  // namespace AY

  Decoder::Ptr CreateAYEMULDecoder();
}  // namespace Formats::Chiptune
