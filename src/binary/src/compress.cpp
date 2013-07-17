/*
Abstract:
  Binary data compress/decompress functions implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//common includes
#include <error.h>
//library includes
#include <binary/compress.h>
//3rd-party includes
#include <3rdparty/zlib/zlib.h>

#define FILE_TAG 965A492C

namespace Binary
{
  namespace Compression
  {
    namespace Zlib
    {
      std::size_t CalculateCompressedSizeUpperBound(std::size_t inputSize)
      {
        return ::compressBound(static_cast<uLong>(inputSize));
      }

      uint8_t* Compress(const uint8_t* inBegin, const uint8_t* inEnd, uint8_t* outBegin, uint8_t* outEnd)
      {
        const uLong srcLen = static_cast<uLong>(inEnd - inBegin);
        uLong dstLen = static_cast<uLongf>(outEnd - outBegin);
        if (const int res = ::compress2(static_cast<Bytef*>(outBegin), &dstLen, static_cast<const Bytef*>(inBegin), srcLen, Z_BEST_COMPRESSION))
        {
          throw Error(THIS_LINE, zError(res));
        }
        return outBegin + dstLen;
      }

      //! @throws Error in case of error
      uint8_t* Decompress(const uint8_t* inBegin, const uint8_t* inEnd, uint8_t* outBegin, uint8_t* outEnd)
      {
        const uLong srcLen = static_cast<uLong>(inEnd - inBegin);
        uLong dstLen = static_cast<uLongf>(outEnd - outBegin);
        if (const int res = ::uncompress(static_cast<Bytef*>(outBegin), &dstLen, static_cast<const Bytef*>(inBegin), srcLen))
        {
          throw Error(THIS_LINE, zError(res));
        }
        return outBegin + dstLen;
      }
    }
  }
}
