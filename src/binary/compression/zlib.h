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

//common includes
#include <types.h>

namespace Binary
{
  namespace Compression
  {
    namespace Zlib
    {
      //! @throws Error
      //! @return real output size
      std::size_t DecompressRaw(const void* input, std::size_t inputSize, void* output, std::size_t maxOutputSize);
      
      //! @throws Error
      //! @return real output size
      std::size_t Decompress(const void* input, std::size_t inputSize, void* output, std::size_t maxOutputSize);
    }
  }
}
