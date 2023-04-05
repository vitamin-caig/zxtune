/**
 *
 * @file
 *
 * @brief  Image-related utilities
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <types.h>
// library includes
#include <binary/container.h>
#include <binary/view.h>

namespace Formats
{
  struct CHS
  {
    // all elements are 0-based
    CHS()
      : Cylinder()
      , Head()
      , Sector()
    {}

    CHS(uint_t c, uint_t h, uint_t s)
      : Cylinder(c)
      , Head(h)
      , Sector(s)
    {}

    uint_t Cylinder;
    uint_t Head;
    uint_t Sector;
  };

  class ImageBuilder
  {
  public:
    typedef std::shared_ptr<ImageBuilder> Ptr;
    virtual ~ImageBuilder() = default;

    virtual void SetGeometry(const CHS& geometry) = 0;
    virtual void SetSector(const CHS& location, Binary::View data) = 0;

    virtual Binary::Container::Ptr GetResult() const = 0;
  };

  // Puts all sectors sequentally skipping not specified
  ImageBuilder::Ptr CreateSparsedImageBuilder();
}  // namespace Formats
