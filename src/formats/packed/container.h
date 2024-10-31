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

#include "formats/packed.h"

#include "types.h"

namespace Formats::Packed
{
  Container::Ptr CreateContainer(Binary::Container::Ptr data, std::size_t origSize);
}  // namespace Formats::Packed
