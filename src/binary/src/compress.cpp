/**
*
* @file
*
* @brief  Compress/decompress functions implementation
*
* @author vitamin.caig@gmail.com
*
**/

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

      Dump Uncompress(const void* packedData, std::size_t packedSize, std::size_t unpackedSizeHint)
      {
        z_stream stream = z_stream();
        if (const auto res = ::inflateInit(&stream))
        {
          throw Error(THIS_LINE, zError(res));
        }
        const std::shared_ptr<void> cleanup(&stream, ::inflateEnd);

        Dump result(unpackedSizeHint ? unpackedSizeHint : packedSize * 2);
        
        stream.next_in = static_cast<Bytef*>(const_cast<void*>(packedData));
        stream.avail_in = static_cast<uInt>(packedSize);
        stream.next_out = static_cast<Bytef*>(&result.front());
        stream.avail_out = static_cast<uInt>(result.size());
        for (;;)
        {
          if (stream.avail_out == 0)
          {
            const auto curSize = result.size();
            const auto addSize = 2 * stream.avail_in;
            result.resize(curSize + addSize);
            stream.next_out = static_cast<Bytef*>(&result.front() + curSize);
            stream.avail_out = static_cast<uInt>(addSize);
          }
          const int res = ::inflate(&stream, Z_SYNC_FLUSH);
          if (Z_STREAM_END == res)
          {
            result.resize(stream.total_out);
            result.shrink_to_fit();
            return result;
          }
          else if (Z_OK != res)
          { 
            throw Error(THIS_LINE, zError(res));
          }
        }
      }
    }
  }
}
