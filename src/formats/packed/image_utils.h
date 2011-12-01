/*
Abstract:
  Different image-related utils

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef FORMATS_PACKED_IMAGE_UTILS_H_DEFINED
#define FORMATS_PACKED_IMAGE_UTILS_H_DEFINED

//common includes
#include <types.h>
//library includes
#include <binary/container.h>
//boost includes
#include <boost/shared_ptr.hpp>

namespace Formats
{
  struct CHS
  {
    //all elements are 0-based
    CHS()
      : Cylinder()
      , Head()
      , Sector()
    {
    }

    CHS(uint_t c, uint_t h, uint_t s)
      : Cylinder(c)
      , Head(h)
      , Sector(s)
    {
    }

    uint_t Cylinder;
    uint_t Head;
    uint_t Sector;
  };

  class ImageBuilder
  {
  public:
    typedef boost::shared_ptr<ImageBuilder> Ptr;
    virtual ~ImageBuilder() {}

    virtual void SetGeometry(const CHS& geometry) = 0;
    virtual void SetSector(const CHS& location, const Dump& data) = 0;

    virtual Binary::Container::Ptr GetResult() const = 0;
  };

  //Puts all sectors sequentally skipping not specified
  ImageBuilder::Ptr CreateSparsedImageBuilder();
}

#endif //FORMATS_PACKED_IMAGE_UTILS_H_DEFINED
