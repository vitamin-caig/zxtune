/**
 *
 * @file
 *
 * @brief  Multitrack archives support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "formats/archived/multitrack/multitrack.h"
#include "formats/archived/multitrack/filename.h"
// common includes
#include <make_ptr.h>
// library includes
#include <binary/container_base.h>
#include <strings/prefixed_index.h>
// std includes
#include <utility>

namespace Formats::Archived
{
  namespace MultitrackArchives
  {
    const auto FILENAME_PREFIX = "#"_sv;

    String CreateFilename(std::size_t index)
    {
      return Strings::PrefixedIndex(FILENAME_PREFIX, index).ToString();
    }

    std::optional<std::size_t> ParseFilename(StringView str)
    {
      const auto parsed = Strings::PrefixedIndex(FILENAME_PREFIX, str);
      std::optional<std::size_t> result;
      if (parsed.IsValid())
      {
        result = parsed.GetIndex();
      }
      return result;
    }

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

    class Container : public Binary::BaseContainer<Archived::Container, Multitrack::Container>
    {
    public:
      explicit Container(Multitrack::Container::Ptr data)
        : BaseContainer(std::move(data))
      {}

      void ExploreFiles(const Container::Walker& walker) const override
      {
        for (uint_t idx = 0, total = CountFiles(); idx < total; ++idx)
        {
          const uint_t song = idx + 1;
          const String subPath = CreateFilename(song);
          const Binary::Container::Ptr subData = Delegate->WithStartTrackIndex(idx);
          const File file(subPath, subData);
          walker.OnFile(file);
        }
      }

      File::Ptr FindFile(const String& name) const override
      {
        const auto filename = ParseFilename(name);
        if (!filename)
        {
          return File::Ptr();
        }
        const uint_t song = *filename;
        if (song < 1 || song > CountFiles())
        {
          return File::Ptr();
        }
        const uint_t idx = song - 1;
        const Binary::Container::Ptr subData = Delegate->WithStartTrackIndex(idx);
        return MakePtr<File>(name, subData);
      }

      uint_t CountFiles() const override
      {
        return Delegate->TracksCount();
      }
    };
  }  // namespace MultitrackArchives

  class MultitrackArchiveDecoder : public Decoder
  {
  public:
    MultitrackArchiveDecoder(String description, Formats::Multitrack::Decoder::Ptr delegate)
      : Description(std::move(description))
      , Delegate(std::move(delegate))
    {}

    String GetDescription() const override
    {
      return Description;
    }

    Binary::Format::Ptr GetFormat() const override
    {
      return Delegate->GetFormat();
    }

    Container::Ptr Decode(const Binary::Container& rawData) const override
    {
      if (const Formats::Multitrack::Container::Ptr data = Delegate->Decode(rawData))
      {
        if (data->TracksCount() > 1)
        {
          return MakePtr<MultitrackArchives::Container>(data);
        }
      }
      return Container::Ptr();
    }

  private:
    const String Description;
    const Formats::Multitrack::Decoder::Ptr Delegate;
  };

  Decoder::Ptr CreateMultitrackArchiveDecoder(String description, Formats::Multitrack::Decoder::Ptr delegate)
  {
    return MakePtr<MultitrackArchiveDecoder>(std::move(description), std::move(delegate));
  }
}  // namespace Formats::Archived
