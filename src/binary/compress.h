/**
*
* @file
*
* @brief  Binary data compress/decompress functions
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

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
      inline void Compress(const Dump& input, Dump& output)
      {
        Dump result(CalculateCompressedSizeUpperBound(input.size()));
        const uint8_t* const in = &input[0];
        uint8_t* const out = &result[0];
        uint8_t* const outEnd = Compress(in, in + input.size(), out, out + result.size());
        result.resize(outEnd - out);
        output.swap(result);
      }
      
      //! Easy-to-use helper with transactional behaviour
      inline void Decompress(const Dump& input, Dump& output)
      {
        Dump result(output.size());
        const uint8_t* const in = &input[0];
        uint8_t* const out = &result[0];
        Decompress(in, in + input.size(), out, out + output.size());
        output.swap(result);
      }

      //Same as zlib 'compress' and 'uncompress'
      inline Dump Compress(const void* unpackedData, std::size_t unpackedSize)
      {
        Dump result(CalculateCompressedSizeUpperBound(unpackedSize));
        const uint8_t* const in = static_cast<const uint8_t*>(unpackedData);
        uint8_t* const out = &result[0];
        uint8_t* const outEnd = Compress(in, in + unpackedSize, out, out + result.size());
        result.resize(outEnd - out);
        result.shrink_to_fit();
        return result;
      }
      
      Dump Uncompress(const void* packedData, std::size_t packedSize, std::size_t unpackedSizeHint = 0);
    }
  }
}
