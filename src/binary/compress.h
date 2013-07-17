/**
*
* @file     binary/compress.h
* @brief    Binary data compress/decompress functions
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef BINARY_COMPRESS_H_DEFINED
#define BINARY_COMPRESS_H_DEFINED

//common includes
#include <types.h>

namespace Binary
{
  namespace Compression
  {
    namespace Zlib
    {
      std::size_t CalculateCompressedSizeUpperBound(std::size_t inputSize);

      //! @throws Error in case of error
      //! @return End of packed data
      uint8_t* Compress(const uint8_t* inBegin, const uint8_t* inEnd, uint8_t* outBegin, uint8_t* outEnd);

      //! @throws Error in case of error
      //! @return End of unpacked data
      uint8_t* Decompress(const uint8_t* inBegin, const uint8_t* inEnd, uint8_t* outBegin, uint8_t* outEnd);

      //! Easy-to-use helper with transactional behaviour
      void Compress(const Dump& input, Dump& output)
      {
        Dump result(CalculateCompressedSizeUpperBound(input.size()));
        const uint8_t* const in = &input[0];
        uint8_t* const out = &result[0];
        uint8_t* const outEnd = Compress(in, in + input.size(), out, out + result.size());
        result.resize(outEnd - out);
        output.swap(result);
      }
      
      //! Easy-to-use helper with transactional behaviour
      void Decompress(const Dump& input, Dump& output)
      {
        Dump result(output.size());
        const uint8_t* const in = &input[0];
        uint8_t* const out = &result[0];
        Decompress(in, in + input.size(), out, out + output.size());
        output.swap(result);
      }
    }
  }
}

#endif //BINARY_COMPRESS_H_DEFINED
