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

namespace Binary::Compression::Zlib
{
  Container::Ptr CreateDeferredDecompressContainer(Data::Ptr packed, std::size_t unpackedSize);
}  // namespace Binary::Compression::Zlib
