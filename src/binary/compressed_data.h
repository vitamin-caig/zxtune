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

//library includes
#include <binary/container.h>

namespace Binary
{
  namespace Compression
  {
    namespace Zlib
    {
      Container::Ptr CreateDeferredUncompressContainer(Data::Ptr packed, std::size_t unpackedSizeHint = 0);
    }
  }
}
