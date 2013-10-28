/*
Abstract:
  TrDOS catalogue support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "trdos_catalogue.h"
//library includes
#include <binary/container_factories.h>
#include <strings/format.h>
//std includes
#include <cstring>
#include <numeric>
//boost includes
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>

namespace
{
  using namespace TRDos;

  bool AreFilesMergeable(const File& lh, const File& rh)
  {
    const std::size_t firstSize = lh.GetSize();
    //merge if files are sequental
    if (lh.GetOffset() + firstSize != rh.GetOffset())
    {
      return false;
    }
    //and previous file size is multiplication of 255 sectors
    if (0 == (firstSize % 0xff00))
    {
      return true;
    }
    //xxx.x and
    //xxx    '.x should be merged
    static const Char SATTELITE_SEQ[] = {'\'', '.', '\0'};
    const String::size_type apPos(rh.GetName().find(SATTELITE_SEQ));
    const bool isSatelite = apPos != String::npos;
    if (!isSatelite)
    {
      return false;
    }
    const std::size_t secondSize = rh.GetSize();
    //ProDigiTracker
    if (firstSize == 0xc300 && secondSize == 0xc000)
    {
      return true;
    }
    //DigitalStudio
    if (firstSize == 0xf200 && secondSize == 0xc000)
    {
      return true;
    }
    return false;
  }

  typedef std::vector<File::Ptr> FilesList;

  class MultiFile : public File
  {
  public:
    typedef boost::shared_ptr<MultiFile> Ptr;

    virtual bool Merge(File::Ptr other) = 0;

    virtual void SetContainer(Binary::Container::Ptr data) = 0;
  };

  class FixedNameFile : public File
  {
  public:
    FixedNameFile(const String& newName, File::Ptr delegate)
      : FixedName(newName)
      , Delegate(delegate)
    {
    }

    virtual String GetName() const
    {
      return FixedName;
    }

    virtual std::size_t GetSize() const
    {
      return Delegate->GetSize();
    }

    virtual Binary::Container::Ptr GetData() const
    {
      return Delegate->GetData();
    }

    virtual std::size_t GetOffset() const
    {
      return Delegate->GetOffset();
    }
  private:
    const String FixedName;
    const File::Ptr Delegate;
  };

  class NamesGenerator
  {
  public:
    explicit NamesGenerator(const String& name)
      : Name(name)
      , Idx(1)
    {
    }

    String operator()() const
    {
      return GenerateName();
    }

    void operator ++()
    {
      ++Idx;
    }
  private:
    String GenerateName() const
    {
      static const Char DISAMBIG_NAME_FORMAT[] = {'%', '1', '%', '~', '%', '2', '%', '%', '3', '%', '\0'};
      assert(Idx);
      const String::size_type dotPos = Name.find_last_of('.');
      const String base = Name.substr(0, dotPos);
      const String ext = dotPos == String::npos ? String() : Name.substr(dotPos);
      return Strings::Format(DISAMBIG_NAME_FORMAT, base, Idx, ext);
    }
  private:
    const String Name;
    unsigned Idx;
  };

  class CommonCatalogue : public Formats::Archived::Container
  {
  public:
    template<class T>
    CommonCatalogue(Binary::Container::Ptr data, T from, T to)
      : Delegate(data)
      , Files(from, to)
    {
      assert(Delegate);
    }

    //Binary::Container
    virtual const void* Start() const
    {
      return Delegate->Start();
    }

    virtual std::size_t Size() const
    {
      return Delegate->Size();
    }

    virtual Binary::Container::Ptr GetSubcontainer(std::size_t offset, std::size_t size) const
    {
      return Delegate->GetSubcontainer(offset, size);
    }

    //Archive::Container
    virtual void ExploreFiles(const Formats::Archived::Container::Walker& walker) const
    {
      for (FilesList::const_iterator it = Files.begin(), lim = Files.end(); it != lim; ++it)
      {
        walker.OnFile(**it);
      }
    }

    virtual Formats::Archived::File::Ptr FindFile(const String& name) const
    {
      const FilesList::const_iterator it = std::find_if(Files.begin(), Files.end(), boost::bind(&File::GetName, _1) == name);
      if (it == Files.end())
      {
        return Formats::Archived::File::Ptr();
      }
      return *it;
    }

    virtual uint_t CountFiles() const
    {
      return static_cast<uint_t>(Files.size());
    }
  private:
    const Binary::Container::Ptr Delegate;
    const FilesList Files;
  };

  typedef std::vector<MultiFile::Ptr> MultiFilesList;

  class BaseCatalogueBuilder : public CatalogueBuilder
  {
  public:
    virtual void SetRawData(Binary::Container::Ptr data)
    {
      Data = data;
      std::for_each(Files.begin(), Files.end(), boost::bind(&MultiFile::SetContainer, _1, Data));
    }

    virtual void AddFile(File::Ptr newOne)
    {
      if (!Merge(newOne))
      {
        AddNewOne(newOne);
      }
    }

    virtual Formats::Archived::Container::Ptr GetResult() const
    {
      if (Data && !Files.empty())
      {
        return Formats::Archived::Container::Ptr(new CommonCatalogue(Data, Files.begin(), Files.end()));
      }
      else
      {
        return Formats::Archived::Container::Ptr();
      }
    }
  protected:
    virtual MultiFile::Ptr CreateMultiFile(File::Ptr inFile) = 0;
  private:
    bool Merge(File::Ptr newOne) const
    {
      return !Files.empty() && Files.back()->Merge(newOne);
    }

    void AddNewOne(File::Ptr newOne)
    {
      const File::Ptr fixed = CreateUniqueNameFile(newOne);
      const MultiFile::Ptr res = CreateMultiFile(fixed);
      res->SetContainer(Data);
      Files.push_back(res);
    }

    File::Ptr CreateUniqueNameFile(File::Ptr newOne)
    {
      const String originalName(newOne->GetName());
      if (!originalName.empty() &&
          !HasFile(originalName))
      {
        return newOne;
      }
      for (NamesGenerator gen(originalName); ; ++gen)
      {
        const String newName = gen();
        if (!HasFile(newName))
        {
          return boost::make_shared<FixedNameFile>(newName, newOne);
        }
      }
    }

    bool HasFile(const String& name) const
    {
      const MultiFilesList::const_iterator it = std::find_if(Files.begin(), Files.end(), boost::bind(&File::GetName, _1) == name);
      return it != Files.end();
    }
  private:
    Binary::Container::Ptr Data;
    MultiFilesList Files;
  };

  // generic files support
  class GenericFile : public File
  {
  public:
    GenericFile(Binary::Container::Ptr data, const String& name, std::size_t off, std::size_t size)
      : Data(data)
      , Name(name)
      , Offset(off)
      , Size(size)
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
      return Data;
    }

    virtual std::size_t GetOffset() const
    {
      return Offset;
    }
  private:
    const Binary::Container::Ptr Data;
    const String Name;
    const std::size_t Offset;
    const std::size_t Size;
  };

  class GenericMultiFile : public MultiFile
  {
  public:
    explicit GenericMultiFile(File::Ptr delegate)
    {
      Subfiles.push_back(delegate);
    }

    virtual String GetName() const
    {
      return Subfiles.front()->GetName();
    }

    virtual std::size_t GetSize() const
    {
      return 1 == Subfiles.size()
        ? Subfiles.front()->GetSize()
        : std::accumulate(Subfiles.begin(), Subfiles.end(), std::size_t(0), 
          boost::bind(std::plus<std::size_t>(), _1, boost::bind(&File::GetSize, _2)));
    }

    virtual Binary::Container::Ptr GetData() const
    {
      if (1 == Subfiles.size())
      {
        return Subfiles.front()->GetData();
      }
      std::auto_ptr<Dump> res(new Dump(GetSize()));
      uint8_t* dst = &res->front();
      for (FilesList::const_iterator it = Subfiles.begin(), lim = Subfiles.end(); it != lim; ++it)
      {
        const File::Ptr file = *it;
        const Binary::Container::Ptr data = file->GetData();
        if (!data)
        {
          return data;
        }
        const std::size_t size = data->Size();
        assert(size == file->GetSize());
        std::memcpy(dst, data->Start(), size);
        dst += size;
      }
      return Binary::CreateContainer(res);
    }

    virtual std::size_t GetOffset() const
    {
      return Subfiles.front()->GetOffset();
    }

    virtual bool Merge(File::Ptr rh)
    {
      if (AreFilesMergeable(*Subfiles.back(), *rh))
      {
        Subfiles.push_back(rh);
        return true;
      }
      return false;
    }

    virtual void SetContainer(Binary::Container::Ptr /*data*/)
    {
    }
  private:
    FilesList Subfiles;
  };

  //TODO: remove implementation inheritance
  class GenericCatalogueBuilder : public BaseCatalogueBuilder
  {
  public:
    virtual MultiFile::Ptr CreateMultiFile(File::Ptr inFile)
    {
      return boost::make_shared<GenericMultiFile>(inFile);
    }
  };

  //flat files support
  class FlatFile : public File
  {
  public:
    FlatFile(const String& name, std::size_t off, std::size_t size)
      : Name(name)
      , Offset(off)
      , Size(size)
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
      assert(!"Should not be called");
      return Binary::Container::Ptr();
    }

    virtual std::size_t GetOffset() const
    {
      return Offset;
    }
  private:
    const String Name;
    const std::size_t Offset;
    const std::size_t Size;
  };

  class FlatMultiFile : public MultiFile
  {
  public:
    FlatMultiFile(File::Ptr delegate)
      : Delegate(delegate)
      , Size(delegate->GetSize())
    {
    }

    virtual String GetName() const
    {
      return Delegate->GetName();
    }

    virtual std::size_t GetSize() const
    {
      return Size;
    }

    virtual Binary::Container::Ptr GetData() const
    {
      return Data->GetSubcontainer(GetOffset(), Size);
    }

    virtual std::size_t GetOffset() const
    {
      return Delegate->GetOffset();
    }

    virtual bool Merge(File::Ptr rh)
    {
      if (AreFilesMergeable(*this, *rh))
      {
        Size += rh->GetSize();
        return true;
      }
      return false;
    }

    virtual void SetContainer(Binary::Container::Ptr data)
    {
      Data = data;
    }
  private:
    const File::Ptr Delegate;
    std::size_t Size;
    Binary::Container::Ptr Data;
  };

  //TODO: remove implementation inheritance
  class FlatCatalogueBuilder : public BaseCatalogueBuilder
  {
  public:
    virtual MultiFile::Ptr CreateMultiFile(File::Ptr inFile)
    {
      return boost::make_shared<FlatMultiFile>(inFile);
    }
  };
}

namespace TRDos
{
  File::Ptr File::Create(Binary::Container::Ptr data, const String& name, std::size_t off, std::size_t size)
  {
    return boost::make_shared<GenericFile>(data, name, off, size);
  }

  File::Ptr File::CreateReference(const String& name, std::size_t off, std::size_t size)
  {
    return boost::make_shared<FlatFile>(name, off, size);
  }

  CatalogueBuilder::Ptr CatalogueBuilder::CreateGeneric()
  {
    return CatalogueBuilder::Ptr(new GenericCatalogueBuilder());
  }

  CatalogueBuilder::Ptr CatalogueBuilder::CreateFlat()
  {
    return CatalogueBuilder::Ptr(new FlatCatalogueBuilder());
  }
}
