/*
Abstract:
  Zip convertors support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "container_supp_common.h"
#include "core/plugins/registrator.h"
#include "core/src/path.h"
//common includes
#include <logging.h>
#include <tools.h>
//library includes
#include <core/plugin_attrs.h>
#include <core/plugins_parameters.h>
#include <formats/packed_decoders.h>
#include <formats/packed/zip_supp.h>
//std includes
#include <numeric>
//boost includes
#include <boost/make_shared.hpp>
//text includes
#include <core/text/plugins.h>

namespace
{
  using namespace ZXTune;

  const Char ID[] = {'Z', 'I', 'P', '\0'};
  const String VERSION(FromStdString("$Rev$"));
  const Char* const INFO = Text::ZIP_PLUGIN_INFO;
  const uint_t CAPS = CAP_STOR_MULTITRACK;

  const std::string THIS_MODULE("Core::ZIPSupp");

  class ZipContainerFile : public Container::File
  {
  public:
    ZipContainerFile(const Formats::Packed::Decoder& decoder, Binary::Container::Ptr data, const String& name, std::size_t size)
      : Decoder(decoder)
      , Data(data)
      , Name(name)
      , Size(size)
    {
    }

    virtual String GetName() const
    {
      return Name;
    }

    virtual std::size_t GetOffset() const
    {
      return ~std::size_t(0);
    }

    virtual std::size_t GetSize() const
    {
      return Size;
    }

    virtual Binary::Container::Ptr GetData() const
    {
      Log::Debug(THIS_MODULE, "Decompressing '%1%'", Name);
      return Decoder.Decode(Data->Data(), Data->Size());
    }
  private:
    const Formats::Packed::Decoder& Decoder;
    const Binary::Container::Ptr Data;
    const String Name;
    const std::size_t Size;
  };

  class ZipIterator
  {
  public:
    explicit ZipIterator(const Formats::Packed::Decoder& decoder, Binary::Container::Ptr data)
      : Decoder(decoder)
      , Data(data)
      , Limit(Data->Size())
      , Offset(0)
    {
    }

    bool IsEof() const
    {
      const std::size_t dataRest = Limit - Offset;
      if (dataRest < sizeof(Formats::Packed::Zip::LocalFileHeader))
      {
        return true;
      }
      const Formats::Packed::Zip::LocalFileHeader& header = GetHeader();
      if (!header.IsValid())
      {
        return true;
      }
      if (dataRest < header.GetTotalFileSize())
      {
        return true;
      }
      return false;
    }

    bool IsValid() const
    {
      assert(!IsEof());
      const Formats::Packed::Zip::LocalFileHeader& header = GetHeader();
      return header.IsSupported();
    }

    String GetName() const
    {
      assert(!IsEof());
      const Formats::Packed::Zip::LocalFileHeader& header = GetHeader();
      return String(header.Name, header.Name + fromLE(header.NameSize));
    }

    Container::File::Ptr GetFile() const
    {
      assert(IsValid());
      const Formats::Packed::Zip::LocalFileHeader& header = GetHeader();
      const Binary::Container::Ptr data = Data->GetSubcontainer(Offset, header.GetTotalFileSize());
      return boost::make_shared<ZipContainerFile>(Decoder, data, GetName(), fromLE(header.Attributes.UncompressedSize));
    }

    void Next()
    {
      assert(!IsEof());
      Offset += GetHeader().GetTotalFileSize();
    }

    std::size_t GetOffset() const
    {
      return Offset;
    }
  private:
    const Formats::Packed::Zip::LocalFileHeader& GetHeader() const
    {
      assert(Limit - Offset >= sizeof(Formats::Packed::Zip::LocalFileHeader));
      return *safe_ptr_cast<const Formats::Packed::Zip::LocalFileHeader*>(static_cast<const uint8_t*>(Data->Data()) + Offset);
    }
  private:
    const Formats::Packed::Decoder& Decoder;
    const Binary::Container::Ptr Data;
    const std::size_t Limit;
    std::size_t Offset;
  };

  template<class It>
  class ZipFilesIterator : public Container::Catalogue::Iterator
  {
  public:
    ZipFilesIterator(It begin, It limit)
      : Current(begin)
      , Limit(limit)
    {
    }

    virtual bool IsValid() const
    {
      return Current != Limit;
    }

    virtual Container::File::Ptr Get() const
    {
      assert(IsValid());
      return Current->second;
    }

    virtual void Next()
    {
      assert(IsValid());
      ++Current;
    }
  private:
    It Current;
    const It Limit;
  };

  class ZipCatalogue : public Container::Catalogue
  {
  public:
    ZipCatalogue(const Formats::Packed::Decoder& decoder, std::size_t maxFileSize, Binary::Container::Ptr data)
      : Decoder(decoder)
      , Data(data)
      , MaxFileSize(maxFileSize)
    {
    }

    virtual Iterator::Ptr GetFiles() const
    {
      FillCache();
      return Iterator::Ptr(new ZipFilesIterator<FilesMap::const_iterator>(Files.begin(), Files.end()));
    }

    virtual uint_t GetFilesCount() const
    {
      FillCache();
      return Files.size();
    }

    virtual Container::File::Ptr FindFile(const DataPath& path) const
    {
      const String inPath = path.AsString();
      const String firstComponent = path.GetFirstComponent();
      if (inPath == firstComponent)
      {
        return FindFile(firstComponent);
      }
      Log::Debug(THIS_MODULE, "Opening '%1%'", inPath);
      DataPath::Ptr resolved = CreateDataPath(firstComponent);
      for (;;)
      {
        const String filename = resolved->AsString();
        Log::Debug(THIS_MODULE, "Trying '%1%'", filename);
        if (Container::File::Ptr file = FindFile(filename))
        {
          Log::Debug(THIS_MODULE, "Found");
          return file;
        }
        if (filename == inPath)
        {
          return Container::File::Ptr();
        }
        const DataPath::Ptr unresolved = SubstractDataPath(path, *resolved);
        resolved = CreateMergedDataPath(resolved, unresolved->GetFirstComponent());
      }
    }

    virtual std::size_t GetSize() const
    {
      FillCache();
      assert(Iter->IsEof());
      return Iter->GetOffset();
    }
  private:
    void FillCache() const
    {
      FindNonCachedFile(String());
    }

    Container::File::Ptr FindFile(const String& name) const
    {
      if (Container::File::Ptr file = FindCachedFile(name))
      {
        return file;
      }
      return FindNonCachedFile(name);
    }

    Container::File::Ptr FindCachedFile(const String& name) const
    {
      if (Iter.get())
      {
        const FilesMap::const_iterator it = Files.find(name);
        if (it != Files.end())
        {
          return it->second;
        }
      }
      return Container::File::Ptr();
    }

    Container::File::Ptr FindNonCachedFile(const String& name) const
    {
      CreateIterator();
      while (!Iter->IsEof())
      {
        const String fileName = Iter->GetName();
        if (!Iter->IsValid())
        {
          Log::Debug(THIS_MODULE, "Invalid file '%1%'", fileName);
          Iter->Next();
          continue;
        }
        Log::Debug(THIS_MODULE, "Found file '%1%'", fileName);
        const Container::File::Ptr fileObject = Iter->GetFile();
        Iter->Next();
        if (fileObject->GetSize() > MaxFileSize)
        {
          Log::Debug(THIS_MODULE, "File is too big (%1% bytes)", fileObject->GetSize());
          continue;
        }
        Files.insert(FilesMap::value_type(fileName, fileObject));
        if (fileName == name)
        {
          return fileObject;
        }
      }
      return Container::File::Ptr();
    }

    void CreateIterator() const
    {
      if (!Iter.get())
      {
        Iter.reset(new ZipIterator(Decoder, Data));
      }
    }
  private:
    const Formats::Packed::Decoder& Decoder;
    const Binary::Container::Ptr Data;
    const std::size_t MaxFileSize;
    mutable std::auto_ptr<ZipIterator> Iter;
    typedef std::map<String, Container::File::Ptr> FilesMap;
    mutable FilesMap Files;
  };

  class ZipFactory : public ContainerFactory
  {
  public:
    ZipFactory()
      : Decoder(Formats::Packed::CreateZipDecoder())
    {
    }

    virtual Binary::Format::Ptr GetFormat() const
    {
      return Decoder->GetFormat();
    }

    virtual Container::Catalogue::Ptr CreateContainer(const Parameters::Accessor& parameters, Binary::Container::Ptr data) const
    {
      if (!Decoder->Check(data->Data(), data->Size()))
      {
        return Container::Catalogue::Ptr();
      }
      Parameters::IntType maxFileSize = Parameters::ZXTune::Core::Plugins::ZIP::MAX_DEPACKED_FILE_SIZE_MB_DEFAULT;
      parameters.FindIntValue(Parameters::ZXTune::Core::Plugins::ZIP::MAX_DEPACKED_FILE_SIZE_MB, maxFileSize);
      maxFileSize *= 1 << 20;
      if (maxFileSize > std::numeric_limits<std::size_t>::max())
      {
        maxFileSize = std::numeric_limits<std::size_t>::max();
      }
      return boost::make_shared<ZipCatalogue>(*Decoder, static_cast<std::size_t>(maxFileSize), data);
    }
  private:
    const Formats::Packed::Decoder::Ptr Decoder;
  };
}

namespace ZXTune
{
  void RegisterZipContainer(PluginsRegistrator& registrator)
  {
    const ContainerFactory::Ptr factory = boost::make_shared<ZipFactory>();
    const ArchivePlugin::Ptr plugin = CreateContainerPlugin(ID, INFO, VERSION, CAPS, factory);
    registrator.RegisterPlugin(plugin);
  }
}
