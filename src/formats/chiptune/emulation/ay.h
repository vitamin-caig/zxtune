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

//common includes
#include <types.h>
//library includes
#include <formats/chiptune.h>

namespace Formats
{
  namespace Chiptune
  {
    namespace AY
    {
      class Builder
      {
      public:
        virtual ~Builder() {}

        virtual void SetTitle(const String& title) = 0;
        virtual void SetAuthor(const String& author) = 0;
        virtual void SetComment(const String& comment) = 0;
        virtual void SetDuration(uint_t total, uint_t fadeout) = 0;
        virtual void SetRegisters(uint16_t reg, uint16_t sp) = 0;
        virtual void SetRoutines(uint16_t init, uint16_t play) = 0;
        virtual void AddBlock(uint16_t addr, const void* data, std::size_t size) = 0;
      };

      uint_t GetModulesCount(const Binary::Container& data);
      Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, std::size_t idx, Builder& target);
      Builder& GetStubBuilder();

      class BlobBuilder : public Builder
      {
      public:
        typedef std::shared_ptr<BlobBuilder> Ptr;

        virtual Binary::Container::Ptr Result() const = 0;
      };

      BlobBuilder::Ptr CreateMemoryDumpBuilder();
      BlobBuilder::Ptr CreateFileBuilder();
    }

    Decoder::Ptr CreateAYEMULDecoder();
  }
}
