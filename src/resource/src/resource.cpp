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
#include <binary/data_builder.h>
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
    explicit LightweightBinaryContainer(Binary::View data)
      : Data(data)
    {}

    const void* Start() const override
    {
      return Data.Start();
    }

    std::size_t Size() const override
    {
      return Data.Size();
    }

    Ptr GetSubcontainer(std::size_t offset, std::size_t size) const override
    {
      return Binary::CreateContainer(Data.SubView(offset, size));
    }

  private:
    const Binary::View Data;
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
      return {};
    }

    void ExploreFiles(const Formats::Archived::Container::Walker& walker) const override
    {
      for (const auto& delegate : Delegates)
      {
        delegate->ExploreFiles(walker);
      }
    }

    Formats::Archived::File::Ptr FindFile(StringView name) const override
    {
      for (const auto& delegate : Delegates)
      {
        if (auto res = delegate->FindFile(name))
        {
          return res;
        }
      }
      return {};
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

  Binary::Data::Ptr ReadFile(const String& filename)
  {
    std::ifstream file(filename.c_str(), std::ios::binary);
    if (!file)
    {
      throw MakeFormattedError(THIS_LINE, translate("Failed to load resource archive '{}'."), filename);
    }
    file.seekg(0, std::ios_base::end);
    const auto size = static_cast<std::size_t>(file.tellg());
    file.seekg(0);
    Binary::DataBuilder res(size);
    file.read(static_cast<char*>(res.Allocate(size)), size);
    return res.CaptureResult();
  }

  Binary::Data::Ptr LoadArchiveContainer()
  {
    const auto filename = GetArchiveContainerName();
    return ReadFile(filename);
  }

  ArchivesSet FindArchives(Binary::View data, const Formats::Archived::Decoder& decoder)
  {
    const auto format = decoder.GetFormat();
    ArchivesSet result;
    for (std::size_t offset = 0, limit = data.Size(); offset < limit;)
    {
      const LightweightBinaryContainer archData(data.SubView(offset));
      if (format->Match(archData))
      {
        if (auto arch = decoder.Decode(archData))
        {
          const auto size = arch->Size();
          Dbg("Found resource archive at {}, size {}", offset, size);
          result.push_back(std::move(arch));
          offset += size;
          continue;
        }
      }
      offset += format->NextMatchOffset(archData);
    }
    return result;
  }

  Formats::Archived::Container::Ptr FindArchive(Binary::View data, const Formats::Archived::Decoder& decoder)
  {
    auto archives = FindArchives(data, decoder);
    switch (archives.size())
    {
    case 0:
      throw Error(THIS_LINE, translate("Failed to find resource archive."));
    case 1:
      return archives.front();
    default:
      return MakePtr<CompositeArchive>(std::move(archives));
    }
  }

  Formats::Archived::Container::Ptr LoadEmbeddedArchive()
  {
    const auto data = LoadArchiveContainer();
    const auto decoder = Formats::Archived::CreateZipDecoder();
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
      if (const auto file = Archive->FindFile(name))
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
