/**
 *
 * @file
 *
 * @brief  PKWare implode (ZIP compression method 6) decompress function
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "binary/container.h"
#include "binary/view.h"

namespace Formats::Archived::Zip
{
  //! @throws Error
  Binary::Container::Ptr Deimplode(Binary::View packed, std::size_t unpackedSize, unsigned flags);
}  // namespace Formats::Archived::Zip
