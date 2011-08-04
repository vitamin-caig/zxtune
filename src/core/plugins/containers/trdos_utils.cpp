/*
Abstract:
  TrDOS utils implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "trdos_utils.h"
#include <core/plugins/utils.h>
//common includes
#include <format.h>
//std includes
#include <numeric>
//boost includes
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>

namespace
{
  using namespace ZXTune;
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
    return apPos != String::npos &&
      (firstSize == 0xc300 && rh.GetSize() == 0xc000); //PDT sattelites sizes
  }

  typedef std::vector<File::Ptr> FilesList;

  class MultiFile : public File
  {
  public:
    typedef boost::shared_ptr<MultiFile> Ptr;

    virtual bool Merge(File::Ptr other) = 0;
  };

  class FixedNameFile : public File
  {
  public:
    FixedNameFile(const String& newName, File::Ptr delegate)
      : NewName(newName)
      , Delegate(delegate)
    {
    }

    virtual String GetName() const
    {
      return NewName;
    }

    virtual std::size_t GetOffset() const
    {
      return Delegate->GetOffset();
    }

    virtual std::size_t GetSize() const
    {
      return Delegate->GetSize();
    }

    virtual IO::DataContainer::Ptr GetData() const
    {
      return Delegate->GetData();
    }
  private:
    const String NewName;
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

  class FilesIterator : public Catalogue::Iterator
  {
  public:
    explicit FilesIterator(const FilesList& files)
      : Current(files.begin())
      , Limit(files.end())
    {
    }

    virtual bool IsValid() const
    {
      return Current != Limit;
    }

    virtual File::Ptr Get() const
    {
      assert(IsValid());
      return *Current;
    }

    virtual void Next()
    {
      assert(IsValid());
      ++Current;
    }
  private:
    FilesList::const_iterator Current;
    const FilesList::const_iterator Limit;
  };

  class CommonCatalogue : public Catalogue
  {
  public:
    template<class T>
    CommonCatalogue(T from, T to, std::size_t usedSize)
      : Files(from, to)
      , UsedSize(usedSize)
    {
      assert(UsedSize);
    }

    virtual Iterator::Ptr GetFiles() const
    {
      return Iterator::Ptr(new FilesIterator(Files));
    }

    virtual uint_t GetFilesCount() const
    {
      return static_cast<uint_t>(Files.size());
    }

    virtual File::Ptr FindFile(const String& name) const
    {
      const FilesList::const_iterator it = std::find_if(Files.begin(), Files.end(), boost::bind(&File::GetName, _1) == name);
      if (it == Files.end())
      {
        return File::Ptr();
      }
      return *it;
    }

    virtual std::size_t GetUsedSize() const
    {
      return UsedSize;
    }
  private:
    const FilesList Files;
    const std::size_t UsedSize;
  };

  typedef std::vector<MultiFile::Ptr> MultiFilesList;

  class BaseCatalogueBuilder : public CatalogueBuilder
  {
  public:
    BaseCatalogueBuilder()
      : UsedSize()
    {
    }

    virtual void SetUsedSize(std::size_t size)
    {
      UsedSize = size;
    }

    virtual void AddFile(File::Ptr newOne)
    {
      if (!Merge(newOne))
      {
        AddNewOne(newOne);
      }
    }

    virtual Catalogue::Ptr GetResult() const
    {
      return Catalogue::Ptr(new CommonCatalogue(Files.begin(), Files.end(), UsedSize));
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
    MultiFilesList Files;
    std::size_t UsedSize;
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

    virtual std::size_t GetOffset() const
    {
      return Subfiles.front()->GetOffset();
    }

    virtual std::size_t GetSize() const
    {
      return std::accumulate(Subfiles.begin(), Subfiles.end(), std::size_t(0), 
        boost::bind(std::plus<std::size_t>(), _1, boost::bind(&File::GetSize, _2)));
    }

    virtual IO::DataContainer::Ptr GetData() const
    {
      std::auto_ptr<Dump> res(new Dump(GetSize()));
      uint8_t* dst = &res->front();
      for (FilesList::const_iterator it = Subfiles.begin(), lim = Subfiles.end(); it != lim; ++it)
      {
        const File::Ptr file = *it;
        const IO::DataContainer::Ptr data = file->GetData();
        const std::size_t size = data->Size();
        assert(size == file->GetSize());
        std::memcpy(dst, data->Data(), size);
        dst += size;
      }
      return IO::CreateDataContainer(res);
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
  private:
    FilesList Subfiles;
  };

  class GenericFile : public File
  {
  public:
    GenericFile(IO::DataContainer::Ptr data, const String& name, std::size_t off, std::size_t size)
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

    virtual std::size_t GetOffset() const
    {
      return Offset;
    }

    virtual std::size_t GetSize() const
    {
      return Size;
    }

    virtual IO::DataContainer::Ptr GetData() const
    {
      return Data->GetSubcontainer(Offset, Size);
    }
  private:
    const IO::DataContainer::Ptr Data;
    const String Name;
    const std::size_t Offset;
    const std::size_t Size;
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
}

namespace TRDos
{
  String GetEntryName(const char (&name)[8], const char (&type)[3])
  {
    static const Char FORBIDDEN_SYMBOLS[] = {'\\', '/', '\?', '\0'};
    static const Char TRDOS_REPLACER('_');
    const String& strName = FromCharArray(name);
    String fname(OptimizeString(strName, TRDOS_REPLACER));
    std::replace_if(fname.begin(), fname.end(), boost::algorithm::is_any_of(FORBIDDEN_SYMBOLS), TRDOS_REPLACER);
    if (!std::isalnum(type[0]))
    {
      return fname;
    }
    fname += '.';
    const char* const invalidSym = std::find_if(type, ArrayEnd(type), !boost::algorithm::is_alnum());
    fname += String(type, invalidSym);
    return fname;
  }

  File::Ptr File::Create(ZXTune::IO::DataContainer::Ptr data, const String& name, std::size_t off, std::size_t size)
  {
    return boost::make_shared<GenericFile>(data, name, off, size);
  }

  CatalogueBuilder::Ptr CatalogueBuilder::CreateGeneric()
  {
    return CatalogueBuilder::Ptr(new GenericCatalogueBuilder());
  }
}
