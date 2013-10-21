/*
Abstract:
  Image container helper

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef FORMATS_IMAGE_CONTAINER_H_DEFINED
#define FORMATS_IMAGE_CONTAINER_H_DEFINED

//library includes
#include <binary/container_factories.h>
#include <formats/image.h>
//boost includes
#include <boost/make_shared.hpp>

class ImageContainer : public Formats::Image::Container
{
public:
  ImageContainer(Binary::Container::Ptr delegate, std::size_t origSize)
    : Delegate(delegate)
    , OrigSize(origSize)
  {
    assert(origSize && delegate && delegate->Size());
  }

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

  virtual std::size_t OriginalSize() const
  {
    return OrigSize;
  }
private:
  const Binary::Container::Ptr Delegate;
  const std::size_t OrigSize;
};

inline ImageContainer::Ptr CreateImageContainer(Binary::Container::Ptr data, std::size_t origSize)
{
  return origSize && data && data->Size()
    ? boost::make_shared<ImageContainer>(data, origSize)
    : ImageContainer::Ptr();
}

inline ImageContainer::Ptr CreateImageContainer(std::auto_ptr<Dump> data, std::size_t origSize)
{
  const Binary::Container::Ptr container = Binary::CreateContainer(data);
  return CreateImageContainer(container, origSize);
}

#endif //FORMATS_IMAGE_CONTAINER_H_DEFINED
