/**
 *
 * @file
 *
 * @brief  Packed data container helper
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <types.h>
// library includes
#include <formats/packed.h>

namespace Formats
{
  namespace Packed
  {
    Container::Ptr CreateContainer(Binary::Container::Ptr data, std::size_t origSize);
  }  // namespace Packed
}  // namespace Formats
