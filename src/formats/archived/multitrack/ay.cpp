/**
 *
 * @file
 *
 * @brief  AY/EMUL containers support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// common includes
#include <make_ptr.h>
// library includes
#include <binary/container_base.h>
#include <binary/format_factories.h>
#include <formats/archived/decoders.h>
#include <formats/archived/multitrack/filename.h>
#include <formats/chiptune/emulation/ay.h>
#include <strings/prefixed_index.h>
// std includes
#include <algorithm>
#include <utility>

namespace Formats::Archived
{
  namespace MultiAY
  {
    class File : public Archived::File
    {
    public:
      File(String name, Binary::Container::Ptr data)
        : Name(std::move(name))
        , Data(std::move(data))
      {}

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
      {}

      void ExploreFiles(const Container::Walker& walker) const override
      {
        for (uint_t idx = 0, total = CountFiles(); idx < total; ++idx)
        {
          const Formats::Chiptune::AY::BlobBuilder::Ptr builder = Formats::Chiptune::AY::CreateFileBuilder();
          if (const Formats::Chiptune::Container::Ptr parsed = Formats::Chiptune::AY::Parse(*Delegate, idx, *builder))
          {
            const String subPath = MultitrackArchives::CreateFilename(idx);
            const Binary::Container::Ptr subData = builder->Result();
            const File file(subPath, subData);
            walker.OnFile(file);
          }
        }
      }

      File::Ptr FindFile(StringView name) const override
      {
        const Strings::PrefixedIndex rawName("@"_sv, name);
        const auto ayIndex = MultitrackArchives::ParseFilename(name);
        if (!rawName.IsValid() && !ayIndex)
        {
          return {};
        }
        const uint_t index = rawName.IsValid() ? rawName.GetIndex() : *ayIndex;
        const uint_t subModules = Formats::Chiptune::AY::GetModulesCount(*Delegate);
        if (subModules < index)
        {
          return {};
        }
        const auto builder = rawName.IsValid() ? Formats::Chiptune::AY::CreateMemoryDumpBuilder()
                                               : Formats::Chiptune::AY::CreateFileBuilder();
        if (!Formats::Chiptune::AY::Parse(*Delegate, index, *builder))
        {
          return File::Ptr();
        }
        auto data = builder->Result();
        return MakePtr<File>(name.to_string(), std::move(data));
      }

      uint_t CountFiles() const override
      {
        return Formats::Chiptune::AY::GetModulesCount(*Delegate);
      }
    };

    const Char DESCRIPTION[] = "Multi-AY/EMUL";
    const auto HEADER_FORMAT =
        "'Z'X'A'Y"  // uint8_t Signature[4];
        "'E'M'U'L"  // only one type is supported now
        ""_sv;
  }  // namespace MultiAY

  class MultiAYDecoder : public Decoder
  {
  public:
    MultiAYDecoder()
      : Format(Binary::CreateFormat(MultiAY::HEADER_FORMAT))
    {}

    String GetDescription() const override
    {
      return MultiAY::DESCRIPTION;
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
}  // namespace Formats::Archived
