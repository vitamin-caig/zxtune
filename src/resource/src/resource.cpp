/**
 *
 * @file
 *
 * @brief  ZIP-based resources implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// common includes
#include <error_tools.h>
#include <make_ptr.h>
#include <pointers.h>
// library includes
#include <binary/container_factories.h>
#include <debug/log.h>
#include <formats/archived/decoders.h>
#include <l10n/api.h>
#include <platform/tools.h>
#include <resource/api.h>
// std includes
#include <fstream>

namespace
{
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("resource");
  const Debug::Stream Dbg("Resource");
}  // namespace

namespace
{
  class LightweightBinaryContainer : public Binary::Container
  {
  public:
    LightweightBinaryContainer(const uint8_t* data, std::size_t size)
      : RawData(data)
      , RawSize(size)
    {}

    const void* Start() const override
    {
      return RawData;
    }

    std::size_t Size() const override
    {
      return RawSize;
    }

    Ptr GetSubcontainer(std::size_t offset, std::size_t size) const override
    {
      std::unique_ptr<Binary::Dump> copy(
          new Binary::Dump(RawData + offset, RawData + std::min(RawSize, offset + size)));
      return Binary::CreateContainer(std::move(copy));
    }

  private:
    const uint8_t* const RawData;
    const std::size_t RawSize;
  };

  typedef std::vector<Formats::Archived::Container::Ptr> ArchivesSet;

  class CompositeArchive : public Formats::Archived::Container
  {
  public:
    explicit CompositeArchive(ArchivesSet delegates)
      : Delegates(std::move(delegates))
    {}

    const void* Start() const override
    {
      return nullptr;
    }

    std::size_t Size() const override
    {
      return 0;
    }

    Binary::Container::Ptr GetSubcontainer(std::size_t /*offset*/, std::size_t /*size*/) const override
    {
      return Binary::Container::Ptr();
    }

    void ExploreFiles(const Formats::Archived::Container::Walker& walker) const override
    {
      for (const auto& delegate : Delegates)
      {
        delegate->ExploreFiles(walker);
      }
    }

    Formats::Archived::File::Ptr FindFile(const String& name) const override
    {
      for (const auto& delegate : Delegates)
      {
        if (const Formats::Archived::File::Ptr res = delegate->FindFile(name))
        {
          return res;
        }
      }
      return Formats::Archived::File::Ptr();
    }

    uint_t CountFiles() const override
    {
      uint_t res = 0;
      for (const auto& delegate : Delegates)
      {
        res += delegate->CountFiles();
      }
      return res;
    }

  private:
    const ArchivesSet Delegates;
  };
}  // namespace

namespace
{
  String GetArchiveContainerName()
  {
    return Platform::GetCurrentImageFilename();
  }

  Binary::Container::Ptr ReadFile(const String& filename)
  {
    std::ifstream file(filename.c_str(), std::ios::binary);
    if (!file)
    {
      throw MakeFormattedError(THIS_LINE, translate("Failed to load resource archive '{}'."), filename);
    }
    file.seekg(0, std::ios_base::end);
    const std::size_t size = static_cast<std::size_t>(file.tellg());
    file.seekg(0);
    std::unique_ptr<Binary::Dump> tmp(new Binary::Dump(size));
    file.read(safe_ptr_cast<char*>(tmp->data()), size);
    return Binary::CreateContainer(std::move(tmp));
  }

  Binary::Container::Ptr LoadArchiveContainer()
  {
    const auto filename = GetArchiveContainerName();
    return ReadFile(filename);
  }

  ArchivesSet FindArchives(const Binary::Container& data, const Formats::Archived::Decoder& decoder)
  {
    const Binary::Format::Ptr format = decoder.GetFormat();
    const std::size_t dataSize = data.Size();
    const uint8_t* const dataStart = static_cast<const uint8_t*>(data.Start());
    ArchivesSet result;
    for (std::size_t offset = 0; offset < dataSize;)
    {
      const LightweightBinaryContainer archData(dataStart + offset, dataSize - offset);
      if (format->Match(archData))
      {
        if (const Formats::Archived::Container::Ptr arch = decoder.Decode(archData))
        {
          const std::size_t size = arch->Size();
          Dbg("Found resource archive at {}, size {}", offset, size);
          result.push_back(arch);
          offset += size;
          continue;
        }
      }
      offset += format->NextMatchOffset(archData);
    }
    return result;
  }

  Formats::Archived::Container::Ptr FindArchive(const Binary::Container& data,
                                                const Formats::Archived::Decoder& decoder)
  {
    const ArchivesSet archives = FindArchives(data, decoder);
    switch (archives.size())
    {
    case 0:
      throw Error(THIS_LINE, translate("Failed to find resource archive."));
    case 1:
      return archives.front();
    default:
      return MakePtr<CompositeArchive>(archives);
    }
  }

  Formats::Archived::Container::Ptr LoadEmbeddedArchive()
  {
    const Binary::Container::Ptr data = LoadArchiveContainer();
    const Formats::Archived::Decoder::Ptr decoder = Formats::Archived::CreateZipDecoder();
    return FindArchive(*data, *decoder);
  }

  class EmbeddedArchive
  {
  public:
    EmbeddedArchive()
      : Archive(LoadEmbeddedArchive())
    {}

    Binary::Container::Ptr Load(const String& name) const
    {
      if (const Formats::Archived::File::Ptr file = Archive->FindFile(name))
      {
        return file->GetData();
      }
      throw MakeFormattedError(THIS_LINE, translate("Failed to load resource file '{}'."), name);
    }

    void Enumerate(Resource::Visitor& visitor) const
    {
      ResourceVisitorAdapter adapter(visitor);
      Archive->ExploreFiles(adapter);
    }

    static const EmbeddedArchive& Instance()
    {
      static const EmbeddedArchive self;
      return self;
    }

  private:
    class ResourceVisitorAdapter : public Formats::Archived::Container::Walker
    {
    public:
      explicit ResourceVisitorAdapter(Resource::Visitor& delegate)
        : Delegate(delegate)
      {}

      void OnFile(const Formats::Archived::File& file) const override
      {
        return Delegate.OnResource(file.GetName());
      }

    private:
      Resource::Visitor& Delegate;
    };

  private:
    const Formats::Archived::Container::Ptr Archive;
  };
}  // namespace

namespace Resource
{
  Binary::Container::Ptr Load(const String& name)
  {
    return EmbeddedArchive::Instance().Load(name);
  }

  void Enumerate(Visitor& visitor)
  {
    return EmbeddedArchive::Instance().Enumerate(visitor);
  }
}  // namespace Resource
