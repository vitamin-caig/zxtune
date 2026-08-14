/**
 *
 * @file
 *
 * @brief  LHA compressor support interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "binary/container.h"

#include "string_type.h"

namespace Binary
{
  class InputStream;
}

namespace Binary::Compression::Lha
{
  // Decoding is processed until input/output data limit
  Container::Ptr DecodeRawData(InputStream& input, const String& method, std::size_t maxOutputSize);

  Container::Ptr DecodeRawData(const Container& input, const String& method, std::size_t maxOutputSize);
}  // namespace Binary::Compression::Lha
