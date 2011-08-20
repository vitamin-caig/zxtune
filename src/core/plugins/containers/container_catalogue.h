/*
Abstract:
  Catalogue suport interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __CORE_PLUGINS_CONTAINERS_CONTAINER_CATALOGUE_H_DEFINED__
#define __CORE_PLUGINS_CONTAINERS_CONTAINER_CATALOGUE_H_DEFINED__

//common includes
#include <iterator.h>
#include <types.h>
//library includes
#include <io/container.h>
//boost includes
#include <boost/shared_ptr.hpp>

namespace Container
{
  class File
  {
  public:
    typedef boost::shared_ptr<const File> Ptr;

    virtual ~File() {}

    virtual String GetName() const = 0;
    virtual std::size_t GetSize() const = 0;
    virtual ZXTune::IO::DataContainer::Ptr GetData() const = 0;
  };

  class Catalogue
  {
  public:
    typedef boost::shared_ptr<const Catalogue> Ptr;
    typedef ObjectIterator<File::Ptr> Iterator;
    virtual ~Catalogue() {}

    virtual Iterator::Ptr GetFiles() const = 0;
    virtual uint_t GetFilesCount() const = 0;
    virtual File::Ptr FindFile(const String& name) const = 0;
    virtual std::size_t GetSize() const = 0;
  };
}
#endif //__CORE_PLUGINS_CONTAINERS_CONTAINER_CATALOGUE_H_DEFINED__
