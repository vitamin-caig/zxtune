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

#include <binary/view.h>

namespace Binary::Compression::Zlib
{
  //! @throws Error
  //! @return real output size
  std::size_t DecompressRaw(View input, void* output, std::size_t maxOutputSize);

  //! @throws Error
  //! @return real output size
  std::size_t Decompress(View input, void* output, std::size_t maxOutputSize);
}  // namespace Binary::Compression::Zlib
