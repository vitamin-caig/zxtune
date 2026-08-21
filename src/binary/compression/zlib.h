/**
 *
 * @file
 *
 * @brief  Binary data compress/decompress functions based on zlib
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "binary/container.h"
#include "binary/view.h"

namespace Binary
{
  class DataBuilder;
  class DataInputStream;
}  // namespace Binary

namespace Binary::Compression::Zlib
{
  //! @throws Error
  Container::Ptr Decompress(View packed, std::size_t unpackedSize = 0 /*unknown*/);

  //! @throws Error
  void Decompress(View packed, DataBuilder& output);

  //! @throws Error
  Container::Ptr DecompressRaw(View packed, std::size_t unpackedSize);

  //! @throws Error
  Container::Ptr DecompressRaw(DataInputStream& packed);

  //! @throws Error
  Container::Ptr Compress(View input);

}  // namespace Binary::Compression::Zlib
