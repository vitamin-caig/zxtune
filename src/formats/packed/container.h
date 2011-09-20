/*
Abstract:
  Packed container helper

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __FORMATS_PACKED_CONTAINER_H_DEFINED__
#define __FORMATS_PACKED_CONTAINER_H_DEFINED__

//library includes
#include <formats/packed.h>
//boost includes
#include <boost/make_shared.hpp>

class PackedContainer : public Formats::Packed::Container
{
public:
  PackedContainer(Binary::Container::Ptr delegate, std::size_t origSize)
    : Delegate(delegate)
    , OriginalSize(origSize)
  {
    assert(origSize && delegate && delegate->Size());
  }

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

  virtual std::size_t PackedSize() const
  {
    return OriginalSize;
  }
private:
  const Binary::Container::Ptr Delegate;
  const std::size_t OriginalSize;
};

inline PackedContainer::Ptr CreatePackedContainer(Binary::Container::Ptr data, std::size_t origSize)
{
  return origSize && data && data->Size()
    ? boost::make_shared<PackedContainer>(data, origSize)
    : PackedContainer::Ptr();
}

inline PackedContainer::Ptr CreatePackedContainer(std::auto_ptr<Dump> data, std::size_t origSize)
{
  const Binary::Container::Ptr container = Binary::CreateContainer(data);
  return CreatePackedContainer(container, origSize);
}

#endif //__FORMATS_PACKED_CONTAINER_H_DEFINED__
