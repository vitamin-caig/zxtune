/**
* 
* @file
*
* @brief  AY/EMUL containers support
*
* @author vitamin.caig@gmail.com
*
**/

//common includes
#include <make_ptr.h>
//library includes
#include <binary/container_base.h>
#include <binary/format_factories.h>
#include <formats/archived/decoders.h>
#include <formats/chiptune/emulation/ay.h>
#include <strings/prefixed_index.h>
//std includes
#include <algorithm>
#include <utility>
//text includes
#include <formats/text/archived.h>

namespace Formats
{
namespace Archived
{
  namespace MultiAY
  {
    class File : public Archived::File
    {
    public:
      File(String name, Binary::Container::Ptr data)
        : Name(std::move(name))
        , Data(std::move(data))
      {
      }

      String GetName() const override
      {
        return Name;
      }

      std::size_t GetSize() const override
      {
        return Data->Size();
      }

      Binary::Container::Ptr GetData() const override
      {
        return Data;
      }
    private:
      const String Name;
      const Binary::Container::Ptr Data;
    };

    class Container : public Binary::BaseContainer<Archived::Container>
    {
    public:
      explicit Container(Binary::Container::Ptr data)
        : BaseContainer(std::move(data))
      {
      }

      void ExploreFiles(const Container::Walker& walker) const override
      {
        for (uint_t idx = 0, total = CountFiles(); idx < total; ++idx)
        {
          const Formats::Chiptune::AY::BlobBuilder::Ptr builder = Formats::Chiptune::AY::CreateFileBuilder();
          if (const Formats::Chiptune::Container::Ptr parsed = Formats::Chiptune::AY::Parse(*Delegate, idx, *builder))
          {
            const String subPath = Strings::PrefixedIndex(Text::MULTITRACK_FILENAME_PREFIX, idx).ToString();
            const Binary::Container::Ptr subData = builder->Result();
            const File file(subPath, subData);
            walker.OnFile(file);
          }
        }
      }

      File::Ptr FindFile(const String& name) const override
      {
        const Strings::PrefixedIndex rawName(Text::AY_RAW_FILENAME_PREFIX, name);
        const Strings::PrefixedIndex ayName(Text::MULTITRACK_FILENAME_PREFIX, name);
        if (!rawName.IsValid() && !ayName.IsValid())
        {
          return File::Ptr();
        }
        const uint_t index = rawName.IsValid() ? rawName.GetIndex() : ayName.GetIndex();
        const uint_t subModules = Formats::Chiptune::AY::GetModulesCount(*Delegate);
        if (subModules < index)
        {
          return File::Ptr();
        }
        const Formats::Chiptune::AY::BlobBuilder::Ptr builder = rawName.IsValid()
          ? Formats::Chiptune::AY::CreateMemoryDumpBuilder()
          : Formats::Chiptune::AY::CreateFileBuilder();
        if (!Formats::Chiptune::AY::Parse(*Delegate, index, *builder))
        {
          return File::Ptr();
        }
        const Binary::Container::Ptr data = builder->Result();
        return MakePtr<File>(name, data);
      }

      uint_t CountFiles() const override
      {
        return Formats::Chiptune::AY::GetModulesCount(*Delegate);
      }
    };

    const StringView HEADER_FORMAT(
      "'Z'X'A'Y" // uint8_t Signature[4];
      "'E'M'U'L" // only one type is supported now
    );
  }//namespace MultiAY

  class MultiAYDecoder : public Decoder
  {
  public:
    MultiAYDecoder()
      : Format(Binary::CreateFormat(MultiAY::HEADER_FORMAT))
    {
    }

    String GetDescription() const override
    {
      return Text::AY_ARCHIVE_DECODER_DESCRIPTION;
    }

    Binary::Format::Ptr GetFormat() const override
    {
      return Format;
    }

    Container::Ptr Decode(const Binary::Container& rawData) const override
    {
      const uint_t subModules = Formats::Chiptune::AY::GetModulesCount(rawData);
      if (subModules < 2)
      {
        return Container::Ptr();
      }
      Formats::Chiptune::AY::Builder& stub = Formats::Chiptune::AY::GetStubBuilder();
      std::size_t maxSize = 0;
      for (uint_t idx = subModules; idx; --idx)
      {
        if (Formats::Chiptune::Container::Ptr ayData = Formats::Chiptune::AY::Parse(rawData, idx - 1, stub))
        {
          maxSize = std::max(maxSize, ayData->Size());
        }
      }
      if (maxSize)
      {
        const Binary::Container::Ptr ayData = rawData.GetSubcontainer(0, maxSize);
        return MakePtr<MultiAY::Container>(ayData);
      }
      else
      {
        return Container::Ptr();
      }
    }
  private:
    const Binary::Format::Ptr Format;
  };

  Decoder::Ptr CreateAYDecoder()
  {
    return MakePtr<MultiAYDecoder>();
  }
}//namespace Archived
}//namespace Formats
