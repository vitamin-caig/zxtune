/**
* 
* @file
*
* @brief  TR-DOS catalogue implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "trdos_catalogue.h"
//common includes
#include <make_ptr.h>
//library includes
#include <binary/container_factories.h>
#include <strings/format.h>
//std includes
#include <cstring>
#include <numeric>
//boost includes
#include <boost/bind.hpp>

namespace TRDos
{
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
    //ExtremeTracker1
    if (firstSize == 0xfc00 && secondSize == 0xbc00)
    {
      return true;
    }
    return false;
  }

  typedef std::vector<File::Ptr> FilesList;

  class MultiFile : public File
  {
  public:
    typedef std::shared_ptr<MultiFile> Ptr;

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

    String GetName() const override
    {
      return FixedName;
    }

    std::size_t GetSize() const override
    {
      return Delegate->GetSize();
    }

    Binary::Container::Ptr GetData() const override
    {
      return Delegate->GetData();
    }

    std::size_t GetOffset() const override
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
    const void* Start() const override
    {
      return Delegate->Start();
    }

    std::size_t Size() const override
    {
      return Delegate->Size();
    }

    Binary::Container::Ptr GetSubcontainer(std::size_t offset, std::size_t size) const override
    {
      return Delegate->GetSubcontainer(offset, size);
    }

    //Archive::Container
    void ExploreFiles(const Formats::Archived::Container::Walker& walker) const override
    {
      for (FilesList::const_iterator it = Files.begin(), lim = Files.end(); it != lim; ++it)
      {
        walker.OnFile(**it);
      }
    }

    Formats::Archived::File::Ptr FindFile(const String& name) const override
    {
      const FilesList::const_iterator it = std::find_if(Files.begin(), Files.end(), boost::bind(&File::GetName, _1) == name);
      if (it == Files.end())
      {
        return Formats::Archived::File::Ptr();
      }
      return *it;
    }

    uint_t CountFiles() const override
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
    void SetRawData(Binary::Container::Ptr data) override
    {
      Data = data;
      std::for_each(Files.begin(), Files.end(), boost::bind(&MultiFile::SetContainer, _1, Data));
    }

    void AddFile(File::Ptr newOne) override
    {
      if (!Merge(newOne))
      {
        AddNewOne(newOne);
      }
    }

    Formats::Archived::Container::Ptr GetResult() const override
    {
      if (Data && !Files.empty())
      {
        return MakePtr<CommonCatalogue>(Data, Files.begin(), Files.end());
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
          return MakePtr<FixedNameFile>(newName, newOne);
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

    String GetName() const override
    {
      return Name;
    }

    std::size_t GetSize() const override
    {
      return Size;
    }

    Binary::Container::Ptr GetData() const override
    {
      return Data;
    }

    std::size_t GetOffset() const override
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

    String GetName() const override
    {
      return Subfiles.front()->GetName();
    }

    std::size_t GetSize() const override
    {
      return 1 == Subfiles.size()
        ? Subfiles.front()->GetSize()
        : std::accumulate(Subfiles.begin(), Subfiles.end(), std::size_t(0), 
          boost::bind(std::plus<std::size_t>(), _1, boost::bind(&File::GetSize, _2)));
    }

    Binary::Container::Ptr GetData() const override
    {
      if (1 == Subfiles.size())
      {
        return Subfiles.front()->GetData();
      }
      std::unique_ptr<Dump> res(new Dump(GetSize()));
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
      return Binary::CreateContainer(std::move(res));
    }

    std::size_t GetOffset() const override
    {
      return Subfiles.front()->GetOffset();
    }

    bool Merge(File::Ptr rh) override
    {
      if (AreFilesMergeable(*Subfiles.back(), *rh))
      {
        Subfiles.push_back(rh);
        return true;
      }
      return false;
    }

    void SetContainer(Binary::Container::Ptr /*data*/) override
    {
    }
  private:
    FilesList Subfiles;
  };

  //TODO: remove implementation inheritance
  class GenericCatalogueBuilder : public BaseCatalogueBuilder
  {
  public:
    MultiFile::Ptr CreateMultiFile(File::Ptr inFile) override
    {
      return MakePtr<GenericMultiFile>(inFile);
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

    String GetName() const override
    {
      return Name;
    }

    std::size_t GetSize() const override
    {
      return Size;
    }

    Binary::Container::Ptr GetData() const override
    {
      assert(!"Should not be called");
      return Binary::Container::Ptr();
    }

    std::size_t GetOffset() const override
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

    String GetName() const override
    {
      return Delegate->GetName();
    }

    std::size_t GetSize() const override
    {
      return Size;
    }

    Binary::Container::Ptr GetData() const override
    {
      return Data->GetSubcontainer(GetOffset(), Size);
    }

    std::size_t GetOffset() const override
    {
      return Delegate->GetOffset();
    }

    bool Merge(File::Ptr rh) override
    {
      if (AreFilesMergeable(*this, *rh))
      {
        Size += rh->GetSize();
        return true;
      }
      return false;
    }

    void SetContainer(Binary::Container::Ptr data) override
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
    MultiFile::Ptr CreateMultiFile(File::Ptr inFile) override
    {
      return MakePtr<FlatMultiFile>(inFile);
    }
  };

  File::Ptr File::Create(Binary::Container::Ptr data, const String& name, std::size_t off, std::size_t size)
  {
    return MakePtr<GenericFile>(data, name, off, size);
  }

  File::Ptr File::CreateReference(const String& name, std::size_t off, std::size_t size)
  {
    return MakePtr<FlatFile>(name, off, size);
  }

  CatalogueBuilder::Ptr CatalogueBuilder::CreateGeneric()
  {
    return MakePtr<GenericCatalogueBuilder>();
  }

  CatalogueBuilder::Ptr CatalogueBuilder::CreateFlat()
  {
    return MakePtr<FlatCatalogueBuilder>();
  }
}
