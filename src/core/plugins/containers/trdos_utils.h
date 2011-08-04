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
//boost includes
#include <boost/shared_ptr.hpp>

namespace TRDos
{
  String GetEntryName(const char (&name)[8], const char (&type)[3]);

  class FileEntry
  {
  public:
    typedef boost::shared_ptr<const FileEntry> Ptr;

    virtual ~FileEntry() {}

    virtual String GetName() const = 0;
    virtual std::size_t GetOffset() const = 0;
    virtual std::size_t GetSize() const = 0;

    static Ptr Create(const String& name, std::size_t off, std::size_t size);
  };

  class FilesSet
  {
  public:
    typedef std::auto_ptr<FilesSet> Ptr;
    typedef ObjectIterator<FileEntry::Ptr> Iterator;
    virtual ~FilesSet() {}

    virtual void AddEntry(FileEntry::Ptr entry) = 0;
    virtual Iterator::Ptr GetEntries() const = 0;
    virtual uint_t GetEntriesCount() const = 0;
    virtual FileEntry::Ptr FindEntry(const String& name) const = 0;

    static Ptr Create();
  };
}
#endif //__CORE_PLUGINS_CONTAINERS_TRDOS_UTILS_H_DEFINED__
