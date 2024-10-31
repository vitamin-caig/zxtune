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

#include <memory>

namespace Formats::Image
{
  Container::Ptr CreateContainer(Binary::Container::Ptr data, std::size_t origSize);
  Container::Ptr CreateContainer(std::unique_ptr<Binary::Dump> data, std::size_t origSize);
}  // namespace Formats::Image
