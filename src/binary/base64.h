/**
*
* @file     binary/base64.h
* @brief    Base64 functions
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef BINARY_BASE64_H_DEFINED
#define BINARY_BASE64_H_DEFINED

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
    inline std::string Encode(const Dump& input)
    {
      std::vector<char> result(CalculateConvertedSize(input.size()));
      const uint8_t* const in = &input[0];
      char* const out = &result[0];
      char* const outEnd = Encode(in, in + input.size(), out, out + result.size());
      return std::string(out, outEnd);
    }

    Dump Decode(const std::string& input);
  }
}

#endif //BINARY_BASE64_H_DEFINED
