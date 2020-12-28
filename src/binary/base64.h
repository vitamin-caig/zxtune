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

//common includes
#include <types.h>

namespace Binary
{
  namespace Base64
  {
    std::size_t CalculateConvertedSize(std::size_t size);

    //! @throws std::exception in case of error
    //! @return End of encoded data
    char* Encode(const uint8_t* inBegin, const uint8_t* inEnd, char* outBegin, char* outEnd);

    //! @throws std::exception in case of error
    //! @return End of decoded data
    uint8_t* Decode(const char* inBegin, const char* inEnd, uint8_t* outBegin, uint8_t* outEnd);

    //easy-to-use wrappers
    inline String Encode(const Dump& input)
    {
      String result(CalculateConvertedSize(input.size()), ' ');
      const uint8_t* const in = input.data();
      char* const out = &result[0];
      Encode(in, in + input.size(), out, out + result.size());
      return result;
    }

    Dump Decode(StringView input);
  }
}
