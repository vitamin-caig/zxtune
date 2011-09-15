/*
Abstract:
  TrDOS catalogue

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __CORE_PLUGINS_CONTAINERS_TRDOS_CATALOGUE_H_DEFINED__
#define __CORE_PLUGINS_CONTAINERS_TRDOS_CATALOGUE_H_DEFINED__

//local includes
#include "container_catalogue.h"
//common includes
#include <types.h>

namespace TRDos
{
  class File : public Container::File
  {
  public:
    typedef boost::shared_ptr<const File> Ptr;

    virtual std::size_t GetOffset() const = 0;

    static Ptr Create(Binary::Container::Ptr data, const String& name, std::size_t off, std::size_t size);
    static Ptr CreateReference(const String& name, std::size_t off, std::size_t size);
  };

  class CatalogueBuilder
  {
  public:
    typedef std::auto_ptr<CatalogueBuilder> Ptr;
    virtual ~CatalogueBuilder() {}

    virtual void SetUsedSize(std::size_t size) = 0;
    virtual void AddFile(File::Ptr file) = 0;

    virtual Container::Catalogue::Ptr GetResult() const = 0;

    static Ptr CreateGeneric();
    static Ptr CreateFlat(Binary::Container::Ptr data);
  };
}
#endif //__CORE_PLUGINS_CONTAINERS_TRDOS_CATALOGUE_H_DEFINED__
