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

namespace
{
  using namespace TRDos;

  bool AreEntriesMergeable(const FileEntry& lh, const FileEntry& rh)
  {
    //merge if files are sequental
    if (lh.Offset + lh.Size != rh.Offset)
    {
      return false;
    }
    //and previous file size is multiplication of 255 sectors
    if (0 == (lh.Size % 0xff00))
    {
      return true;
    }
    //xxx.x and
    //xxx    '.x should be merged
    static const Char SATTELITE_SEQ[] = {'\'', '.', '\0'};
    const String::size_type apPos(rh.Name.find(SATTELITE_SEQ));
    return apPos != String::npos &&
      (lh.Size == 0xc300 && rh.Size == 0xc000); //PDT sattelites sizes
  }

  void MergeEntry(FileEntry& lh, const FileEntry& rh)
  {
    assert(AreEntriesMergeable(lh, rh));
    lh.Size += rh.Size;
  }

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

  typedef std::vector<FileEntry> EntriesList;

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

    virtual FileEntry Get() const
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
    virtual void AddEntry(const FileEntry& newOne)
    {
      if (IsPossibleToMerge(newOne))
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

    virtual const FileEntry* FindEntry(const String& name) const
    {
      const EntriesList::const_iterator it = std::find_if(Entries.begin(), Entries.end(), boost::bind(&FileEntry::Name, _1) == name);
      if (it == Entries.end())
      {
        return 0;
      }
      return &*it;
    }
  private:
    bool IsPossibleToMerge(const FileEntry& newOne) const
    {
      return !Entries.empty() &&
             AreEntriesMergeable(Entries.back(), newOne);
    }

    void MergeWithLast(const FileEntry& newOne)
    {
      MergeEntry(Entries.back(), newOne);
    }

    void AddNewOne(const FileEntry& newOne)
    {
      const String originalName(newOne.Name);
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
          FileEntry fixedEntry(newOne);
          fixedEntry.Name = newName;
          Entries.push_back(fixedEntry);
          return;
        }
      }
    }
  private:
    EntriesList Entries;
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

  FilesSet::Ptr FilesSet::Create()
  {
    return FilesSet::Ptr(new FilesSetImpl());
  }
}
