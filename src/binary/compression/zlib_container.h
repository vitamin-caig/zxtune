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

// library includes
#include <binary/container.h>
#include <binary/view.h>

namespace Binary
{
  namespace Compression
  {
    namespace Zlib
    {
      Container::Ptr CreateDeferredDecompressContainer(Data::Ptr packed, std::size_t unpackedSizeHint = 0);
      Container::Ptr Decompress(View packed, std::size_t unpackedSizeHint = 0);
    }  // namespace Zlib
  }    // namespace Compression
}  // namespace Binary
