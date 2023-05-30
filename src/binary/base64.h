/**
 *
 * @file
 *
 * @brief  Base64 functions
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <types.h>
// library includes
#include <binary/dump.h>
#include <binary/view.h>

namespace Binary::Base64
{
  std::size_t CalculateConvertedSize(std::size_t size);

  //! @throws std::exception in case of error
  //! @return End of encoded data
  char* Encode(const uint8_t* inBegin, const uint8_t* inEnd, char* outBegin, const char* outEnd);

  //! @throws std::exception in case of error
  //! @return End of decoded data
  uint8_t* Decode(const char* inBegin, const char* inEnd, uint8_t* outBegin, const uint8_t* outEnd);

  // easy-to-use wrappers
  inline String Encode(View input)
  {
    String result(CalculateConvertedSize(input.Size()), ' ');
    const auto* const in = input.As<uint8_t>();
    char* const out = &result[0];
    Encode(in, in + input.Size(), out, out + result.size());
    return result;
  }

  Dump Decode(StringView input);
}  // namespace Binary::Base64
