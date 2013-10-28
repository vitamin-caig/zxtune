/*
Abstract:
  ZIP-based resources implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//common includes
#include <error_tools.h>
#include <pointers.h>
//library includes
#include <binary/container_factories.h>
#include <debug/log.h>
#include <formats/archived/decoders.h>
#include <l10n/api.h>
#include <resource/api.h>
#include <platform/tools.h>
//std includes
#include <fstream>
//boost includes
#include <boost/make_shared.hpp>

#define FILE_TAG 82AE713A

namespace
{
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("resource");
  const Debug::Stream Dbg("Resource");
}

namespace
{
  class LightweightBinaryContainer : public Binary::Container
  {
  public:
    LightweightBinaryContainer(const uint8_t* data, std::size_t size)
      : RawData(data)
      , RawSize(size)
    {
    }

    virtual const void* Start() const
    {
      return RawData;
    }

    virtual std::size_t Size() const
    {
      return RawSize;
    }

    virtual Ptr GetSubcontainer(std::size_t offset, std::size_t size) const
    {
      std::auto_ptr<Dump> copy(new Dump(RawData + offset, RawData + std::min(RawSize, offset + size)));
      return Binary::CreateContainer(copy);
    }
  private:
    const uint8_t* const RawData;
    const std::size_t RawSize;
  };

  typedef std::vector<Formats::Archived::Container::Ptr> ArchivesSet;

  class CompositeArchive : public Formats::Archived::Container
  {
  public:
    explicit CompositeArchive(const ArchivesSet& delegates)
      : Delegates(delegates)
    {
    }

    virtual const void* Start() const
    {
      return 0;
    }

    virtual std::size_t Size() const
    {
      return 0;
    }

    virtual Binary::Container::Ptr GetSubcontainer(std::size_t /*offset*/, std::size_t /*size*/) const
    {
      return Binary::Container::Ptr();
    }

    virtual void ExploreFiles(const Formats::Archived::Container::Walker& walker) const
    {
      for (ArchivesSet::const_iterator it = Delegates.begin(), lim = Delegates.end(); it != lim; ++it)
      {
        (*it)->ExploreFiles(walker);
      }
    }

    virtual Formats::Archived::File::Ptr FindFile(const String& name) const
    {
      for (ArchivesSet::const_iterator it = Delegates.begin(), lim = Delegates.end(); it != lim; ++it)
      {
        if (const Formats::Archived::File::Ptr res = (*it)->FindFile(name))
        {
          return res;
        }
      }
      return Formats::Archived::File::Ptr();
    }

    virtual uint_t CountFiles() const
    {
      uint_t res = 0;
      for (ArchivesSet::const_iterator it = Delegates.begin(), lim = Delegates.end(); it != lim; ++it)
      {
        res += (*it)->CountFiles();
      }
      return res;
    }
  private:
    const ArchivesSet Delegates;
  };
}

namespace
{
  std::string GetArchiveContainerName()
  {
    return Platform::GetCurrentImageFilename();
  }

  Binary::Container::Ptr ReadFile(const std::string& filename)
  {
    std::ifstream file(filename.c_str(), std::ios::binary);
    if (!file)
    {
      throw MakeFormattedError(THIS_LINE, translate("Failed to load resource archive '%1%'."), filename);
    }
    file.seekg(0, std::ios_base::end);
    const std::size_t size = file.tellg();
    file.seekg(0);
    std::auto_ptr<Dump> tmp(new Dump(size));
    file.read(safe_ptr_cast<char*>(&tmp->front()), size);
    return Binary::CreateContainer(tmp);
  }

  Binary::Container::Ptr LoadArchiveContainer()
  {
    const std::string filename = GetArchiveContainerName();
    return ReadFile(filename);
  }

  ArchivesSet FindArchives(const Binary::Container& data, const Formats::Archived::Decoder& decoder)
  {
    const Binary::Format::Ptr format = decoder.GetFormat();
    const std::size_t dataSize = data.Size();
    const uint8_t* const dataStart = static_cast<const uint8_t*>(data.Start());
    ArchivesSet result;
    for (std::size_t offset = 0; offset < dataSize; )
    {
      const LightweightBinaryContainer archData(dataStart + offset, dataSize - offset);
      if (format->Match(archData))
      {
        if (const Formats::Archived::Container::Ptr arch = decoder.Decode(archData))
        {
          const std::size_t size = arch->Size();
          Dbg("Found resource archive at %1%, size %2%", offset, size);
          result.push_back(arch);
          offset += size;
          continue;
        }
      }
      offset += format->NextMatchOffset(archData);
    }
    return result;
  }

  Formats::Archived::Container::Ptr FindArchive(const Binary::Container& data, const Formats::Archived::Decoder& decoder)
  {
    const ArchivesSet archives = FindArchives(data, decoder);
    switch (archives.size())
    {
    case 0:
      throw Error(THIS_LINE, translate("Failed to find resource archive."));
    case 1:
      return archives.front();
    default:
      return boost::make_shared<CompositeArchive>(archives);
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
    {
    }

    Binary::Container::Ptr Load(const String& name) const
    {
      if (const Formats::Archived::File::Ptr file = Archive->FindFile(name))
      {
        return file->GetData();
      }
      throw MakeFormattedError(THIS_LINE, translate("Failed to load resource file '%1%'."), name);
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
      {
      }

      virtual void OnFile(const Formats::Archived::File& file) const
      {
        return Delegate.OnResource(file.GetName());
      }
    private:
      Resource::Visitor& Delegate;
    };
  private:
    const Formats::Archived::Container::Ptr Archive;
  };
}

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
}
