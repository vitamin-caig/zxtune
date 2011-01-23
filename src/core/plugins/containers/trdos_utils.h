/*
Abstract:
  TrDOS utils

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __CORE_PLUGINS_CONTAINERS_TRDOS_UTILS_H_DEFINED__
#define __CORE_PLUGINS_CONTAINERS_TRDOS_UTILS_H_DEFINED__

//common includes
#include <iterator.h>
#include <types.h>

namespace TRDos
{
  String GetEntryName(const char (&name)[8], const char (&type)[3]);

  struct FileEntry
  {
    FileEntry()
      : Name(), Offset(), Size()
    {
    }

    FileEntry(const String& name, uint_t off, uint_t sz)
      : Name(name), Offset(off), Size(sz)
    {
    }

    String Name;
    uint_t Offset;
    uint_t Size;
  };

  class FilesSet
  {
  public:
    typedef std::auto_ptr<FilesSet> Ptr;
    typedef ObjectIterator<FileEntry> Iterator;
    virtual ~FilesSet() {}

    virtual void AddEntry(const FileEntry& entry) = 0;
    virtual Iterator::Ptr GetEntries() const = 0;
    virtual uint_t GetEntriesCount() const = 0;
    virtual const FileEntry* FindEntry(const String& name) const = 0;

    static Ptr Create();
  };
}
#endif //__CORE_PLUGINS_CONTAINERS_TRDOS_UTILS_H_DEFINED__
