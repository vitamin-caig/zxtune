/**
 *
 * @file
 *
 * @brief  Binary data compress/decompress functions based on zlib with streaming interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "binary/data_builder.h"
#include "binary/input_stream.h"

namespace Binary::Compression::Zlib
{
  //! @throws Error
  void DecompressRaw(DataInputStream& input, DataBuilder& output, std::size_t outputSizeHint = 0);

  //! @throws Error
  void Decompress(DataInputStream& input, DataBuilder& output, std::size_t outputSizeHint = 0);

  //! @throws Error
  void Compress(DataInputStream& input, DataBuilder& output);
}  // namespace Binary::Compression::Zlib
