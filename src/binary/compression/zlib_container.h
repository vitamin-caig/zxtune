/**
 *
 * @file
 *
 * @brief  On-demand uncompressing Binary::Container adapter
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "binary/container.h"
#include "binary/view.h"

#include <cstddef>

namespace Binary::Compression::Zlib
{
  Container::Ptr CreateDeferredDecompressContainer(Data::Ptr packed, std::size_t unpackedSizeHint = 0);
  Container::Ptr Decompress(View packed, std::size_t unpackedSizeHint = 0);
}  // namespace Binary::Compression::Zlib
