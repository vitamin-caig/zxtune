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
//boost includes
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>

namespace
{
  using namespace TRDos;

  bool AreEntriesMergeable(const FileEntry& lh, const FileEntry& rh)
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

  class MergedFileEntry : public FileEntry
  {
  public:
    MergedFileEntry(FileEntry::Ptr first, FileEntry::Ptr second)
      : First(first)
      , Second(second)
    {
    }

    virtual String GetName() const
    {
      return First->GetName();
    }

    virtual std::size_t GetOffset() const
    {
      return First->GetOffset();
    }

    virtual std::size_t GetSize() const
    {
      return First->GetSize() + Second->GetSize();
    }
  private:
    const FileEntry::Ptr First;
    const FileEntry::Ptr Second;
  };

  class FixedFileEntry : public FileEntry
  {
  public:
    FixedFileEntry(const String& newName, FileEntry::Ptr delegate)
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
  private:
    const String NewName;
    const FileEntry::Ptr Delegate;
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

  typedef std::vector<FileEntry::Ptr> EntriesList;

  class FileEntriesIterator : public FilesSet::Iterator
  {
  public:
    explicit FileEntriesIterator(const EntriesList& entries)
      : Current(entries.begin())
      , Limit(entries.end())
    {
    }

    virtual bool IsValid() const
    {
      return Current != Limit;
    }

    virtual FileEntry::Ptr Get() const
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
    EntriesList::const_iterator Current;
    const EntriesList::const_iterator Limit;
  };

  class FilesSetImpl : public FilesSet
  {
  public:
    virtual void AddEntry(FileEntry::Ptr newOne)
    {
      if (IsPossibleToMerge(*newOne))
      {
        MergeWithLast(newOne);
      }
      else
      {
        AddNewOne(newOne);
      }
    }

    virtual Iterator::Ptr GetEntries() const
    {
      return Iterator::Ptr(new FileEntriesIterator(Entries));
    }

    virtual uint_t GetEntriesCount() const
    {
      return static_cast<uint_t>(Entries.size());
    }

    virtual FileEntry::Ptr FindEntry(const String& name) const
    {
      const EntriesList::const_iterator it = std::find_if(Entries.begin(), Entries.end(), boost::bind(&FileEntry::GetName, _1) == name);
      if (it == Entries.end())
      {
        return FileEntry::Ptr();
      }
      return *it;
    }
  private:
    bool IsPossibleToMerge(const FileEntry& newOne) const
    {
      return !Entries.empty() &&
             AreEntriesMergeable(*Entries.back(), newOne);
    }

    void MergeWithLast(FileEntry::Ptr newOne)
    {
      Entries.back() = boost::make_shared<MergedFileEntry>(Entries.back(), newOne);
    }

    void AddNewOne(FileEntry::Ptr newOne)
    {
      const String originalName(newOne->GetName());
      if (!originalName.empty() &&
          !FindEntry(originalName))
      {
        Entries.push_back(newOne);
        return;
      }
      for (NamesGenerator gen(originalName); ; ++gen)
      {
        const String newName = gen();
        if (!FindEntry(newName))
        {
          const FileEntry::Ptr fixedEntry = boost::make_shared<FixedFileEntry>(newName, newOne);
          Entries.push_back(fixedEntry);
          return;
        }
      }
    }
  private:
    EntriesList Entries;
  };

  class SimpleFileEntry : public FileEntry
  {
  public:
    SimpleFileEntry(const String& name, std::size_t off, std::size_t size)
      : Name(name)
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
  private:
    const String Name;
    const std::size_t Offset;
    const std::size_t Size;
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

  FileEntry::Ptr FileEntry::Create(const String& name, std::size_t off, std::size_t size)
  {
    return boost::make_shared<SimpleFileEntry>(name, off, size);
  }

  FilesSet::Ptr FilesSet::Create()
  {
    return FilesSet::Ptr(new FilesSetImpl());
  }
}
