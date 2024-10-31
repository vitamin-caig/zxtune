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

#include "formats/packed.h"

#include "string_type.h"

namespace Formats::Packed::Lha
{
  Container::Ptr DecodeRawData(const Binary::Container& input, const String& method, std::size_t outputSize);
  Container::Ptr DecodeRawDataAtLeast(const Binary::Container& input, const String& method, std::size_t sizeHint);
}  // namespace Formats::Packed::Lha
