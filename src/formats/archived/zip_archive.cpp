/*
Abstract:
  Zip archives support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//common includes
#include <logging.h>
#include <tools.h>
//library includes
#include <formats/archived.h>
#include <formats/packed_decoders.h>
#include <formats/packed/zip_supp.h>
//std includes
#include <map>
#include <numeric>
//boost includes
#include <boost/make_shared.hpp>

namespace Zip
{
  const std::string THIS_MODULE("Formats::Archived::ZIP");

  using namespace Formats;

  class File : public Archived::File
  {
  public:
    File(const Packed::Decoder& decoder, const String& name, std::size_t size, Binary::Container::Ptr data)
      : Decoder(decoder)
      , Name(name)
      , Size(size)
      , Data(data)
    {
    }

    virtual String GetName() const
    {
      return Name;
    }

    virtual std::size_t GetSize() const
    {
      return Size;
    }

    virtual Binary::Container::Ptr GetData() const
    {
      Log::Debug(THIS_MODULE, "Decompressing '%1%'", Name);
      return Decoder.Decode(*Data);
    }
  private:
    const Formats::Packed::Decoder& Decoder;
    const String Name;
    const std::size_t Size;
    const Binary::Container::Ptr Data;
  };

  //TODO: make BlocksIterator
  class FileIterator
  {
  public:
    explicit FileIterator(const Packed::Decoder& decoder, const Binary::Container& data)
      : Decoder(decoder)
      , Data(data)
      , Limit(Data.Size())
      , Offset(0)
    {
    }

    bool IsEof() const
    {
      const std::size_t dataRest = Limit - Offset;
      if (dataRest < sizeof(Packed::Zip::LocalFileHeader))
      {
        return true;
      }
      const Packed::Zip::LocalFileHeader& header = GetHeader();
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
      const Packed::Zip::LocalFileHeader& header = GetHeader();
      return header.IsSupported();
    }

    String GetName() const
    {
      assert(!IsEof());
      const Packed::Zip::LocalFileHeader& header = GetHeader();
      return String(header.Name, header.Name + fromLE(header.NameSize));
    }

    Archived::File::Ptr GetFile() const
    {
      assert(IsValid());
      const Packed::Zip::LocalFileHeader& header = GetHeader();
      const Binary::Container::Ptr data = Data.GetSubcontainer(Offset, header.GetTotalFileSize());
      return boost::make_shared<File>(Decoder, GetName(), fromLE(header.Attributes.UncompressedSize), data);
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
    const Packed::Zip::LocalFileHeader& GetHeader() const
    {
      assert(Limit - Offset >= sizeof(Packed::Zip::LocalFileHeader));
      return *safe_ptr_cast<const Packed::Zip::LocalFileHeader*>(static_cast<const uint8_t*>(Data.Data()) + Offset);
    }
  private:
    const Packed::Decoder& Decoder;
    const Binary::Container& Data;
    const std::size_t Limit;
    std::size_t Offset;
  };

  class Container : public Archived::Container
  {
  public:
    Container(const Packed::Decoder& decoder, Binary::Container::Ptr data, uint_t filesCount)
      : Decoder(decoder)
      , Delegate(data)
      , FilesCount(filesCount)
    {
      Log::Debug(THIS_MODULE, "Found %1% files", filesCount);
    }

    //Binary::Container
    virtual std::size_t Size() const
    {
      return Delegate->Size();
    }

    virtual const void* Data() const
    {
      return Delegate->Data();
    }

    virtual Binary::Container::Ptr GetSubcontainer(std::size_t offset, std::size_t size) const
    {
      return Delegate->GetSubcontainer(offset, size);
    }

    //Archive::Container
    virtual void ExploreFiles(const Archived::Container::Walker& walker) const
    {
      FillCache();
      for (FilesMap::const_iterator it = Files.begin(), lim = Files.end(); it != lim; ++it)
      {
        walker.OnFile(*it->second);
      }
    }

    virtual Archived::File::Ptr FindFile(const String& name) const
    {
      if (Archived::File::Ptr file = FindCachedFile(name))
      {
        return file;
      }
      return FindNonCachedFile(name);
    }

    virtual uint_t CountFiles() const
    {
      return FilesCount;
    }
  private:
    void FillCache() const
    {
      FindNonCachedFile(String());
    }

    Archived::File::Ptr FindCachedFile(const String& name) const
    {
      if (Iter.get())
      {
        const FilesMap::const_iterator it = Files.find(name);
        if (it != Files.end())
        {
          return it->second;
        }
      }
      return Archived::File::Ptr();
    }

    Archived::File::Ptr FindNonCachedFile(const String& name) const
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
        const Archived::File::Ptr fileObject = Iter->GetFile();
        Files.insert(FilesMap::value_type(fileName, fileObject));
        Iter->Next();
        if (fileName == name)
        {
          return fileObject;
        }
      }
      return Archived::File::Ptr();
    }

    void CreateIterator() const
    {
      if (!Iter.get())
      {
        Iter.reset(new FileIterator(Decoder, *Delegate));
      }
    }
  private:
    const Formats::Packed::Decoder& Decoder;
    const Binary::Container::Ptr Delegate;
    const uint_t FilesCount;
    mutable std::auto_ptr<FileIterator> Iter;
    typedef std::map<String, Archived::File::Ptr> FilesMap;
    mutable FilesMap Files;
  };
}

namespace Formats
{
  namespace Archived
  {
    class ZipDecoder : public Decoder
    {
    public:
      ZipDecoder()
        : FileDecoder(Formats::Packed::CreateZipDecoder())
      {
      }

      virtual String GetDescription() const
      {
        return FileDecoder->GetDescription();
      }

      virtual Binary::Format::Ptr GetFormat() const
      {
        return FileDecoder->GetFormat();
      }

      virtual bool Check(const Binary::Container& data) const
      {
        return FileDecoder->Check(data);
      }

      virtual Container::Ptr Decode(const Binary::Container& data) const
      {
        if (!FileDecoder->Check(data))
        {
          return Container::Ptr();
        }
        //TODO: calculate using blocks iterator and central dir
        uint_t filesCount = 0;
        const std::auto_ptr<Zip::FileIterator> iter(new Zip::FileIterator(*FileDecoder, data));
        for (; !iter->IsEof(); iter->Next())
        {
          filesCount += iter->IsValid();
        }
        if (filesCount)
        {
          const Binary::Container::Ptr archive = data.GetSubcontainer(0, iter->GetOffset());
          return boost::make_shared<Zip::Container>(*FileDecoder, archive, filesCount);
        }
        else
        {
          return Container::Ptr();
        }
      }
    private:
      const Formats::Packed::Decoder::Ptr FileDecoder;
    };

    Decoder::Ptr CreateZipDecoder()
    {
      return boost::make_shared<ZipDecoder>();
    }
  }
}

