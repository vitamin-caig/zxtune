/**
 *
 * @file
 *
 * @brief  Image container helper
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <types.h>
// library includes
#include <formats/image.h>

namespace Formats
{
  namespace Image
  {
    Container::Ptr CreateContainer(Binary::Container::Ptr data, std::size_t origSize);
    Container::Ptr CreateContainer(std::unique_ptr<Dump> data, std::size_t origSize);
  }  // namespace Image
}  // namespace Formats
