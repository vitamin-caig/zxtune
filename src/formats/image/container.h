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

#include "binary/dump.h"
#include "formats/image.h"

#include "types.h"

namespace Formats::Image
{
  Container::Ptr CreateContainer(Binary::Container::Ptr data, std::size_t origSize);
  Container::Ptr CreateContainer(Binary::Dump data, std::size_t origSize);
}  // namespace Formats::Image
