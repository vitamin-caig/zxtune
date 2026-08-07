/**
 *
 * @file
 *
 * @brief  AY/EMUL containers support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/emulation/ay.h"

#include "formats/archived/decoders.h"
#include "formats/archived/multitrack/filename.h"

#include "binary/container_base.h"
#include "binary/format_factories.h"
#include "strings/prefixed_index.h"

#include "make_ptr.h"
#include "string_view.h"

#include <algorithm>
#include <utility>

namespace Formats::Archived
{
  namespace MultiAY
  {
    class File : public Archived::File
    {
    public:
      File(StringView name, Binary::Container::Ptr data)
        : Name(name)
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
          const auto builder = Chiptune::AY::CreateFileBuilder();
          if (const auto parsed = Chiptune::AY::Parse(*Delegate, idx, *builder))
          {
            const auto& subPath = MultitrackArchives::CreateFilename(idx);
            auto subData = builder->Result();
            const File file(subPath, std::move(subData));
            walker.OnFile(file);
          }
        }
      }

      File::Ptr FindFile(StringView name) const override
      {
        const auto rawName = Strings::PrefixedIndex::Parse("@"sv, name);
        const auto ayIndex = MultitrackArchives::ParseFilename(name);
        if (!rawName.IsValid() && !ayIndex)
        {
          return {};
        }
        const uint_t index = rawName.IsValid() ? rawName.GetIndex() : *ayIndex;
        const uint_t subModules = Chiptune::AY::GetModulesCount(*Delegate);
        if (subModules < index)
        {
          return {};
        }
        const auto builder = rawName.IsValid() ? Chiptune::AY::CreateMemoryDumpBuilder()
                                               : Chiptune::AY::CreateFileBuilder();
        if (!Chiptune::AY::Parse(*Delegate, index, *builder))
        {
          return {};
        }
        auto data = builder->Result();
        return MakePtr<File>(name, std::move(data));
      }

      uint_t CountFiles() const override
      {
        return Chiptune::AY::GetModulesCount(*Delegate);
      }
    };

    const auto DESCRIPTION = "Multi-AY/EMUL"sv;
    const auto HEADER_FORMAT =
        "'Z'X'A'Y"  // uint8_t Signature[4];
        "'E'M'U'L"  // only one type is supported now
        ""sv;

    class Decoder : public Archived::Decoder
    {
    public:
      StringView GetDescription() const override
      {
        return DESCRIPTION;
      }

      Binary::Format::Ptr GetFormat() const override
      {
        return Format;
      }

      Container::Ptr Decode(const Binary::Container& rawData) const override
      {
        const uint_t subModules = Chiptune::AY::GetModulesCount(rawData);
        if (subModules < 2)
        {
          return {};
        }
        auto& stub = Chiptune::AY::GetStubBuilder();
        std::size_t maxSize = 0;
        for (uint_t idx = subModules; idx; --idx)
        {
          if (auto ayData = Chiptune::AY::Parse(rawData, idx - 1, stub))
          {
            maxSize = std::max(maxSize, ayData->Size());
          }
        }
        if (maxSize)
        {
          auto ayData = rawData.GetSubcontainer(0, maxSize);
          return MakePtr<Container>(std::move(ayData));
        }
        else
        {
          return {};
        }
      }

    private:
      const Binary::Format::Ptr Format = Binary::CreateFormat(HEADER_FORMAT);
    };
  }  // namespace MultiAY

  Decoder::Ptr CreateAYDecoder()
  {
    return MakePtr<MultiAY::Decoder>();
  }
}  // namespace Formats::Archived
