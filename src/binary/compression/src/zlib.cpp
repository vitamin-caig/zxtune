/**
*
* @file
*
* @brief  Compress/decompress functions implementation based on zlib
*
* @author vitamin.caig@gmail.com
*
**/

//common includes
#include <error.h>
//library includes
#include <binary/compression/zlib.h>
#include <binary/compression/zlib_stream.h>
#include <math/numeric.h>
//3rd-party includes
#include <3rdparty/zlib/zlib.h>

#define FILE_TAG 0A028BD4

namespace Binary
{
  namespace Compression
  {
    namespace Zlib
    {
      class Stream
      {
      public:
        Stream()
          : Delegate()
        {
        }
        
        Stream(const void* input, std::size_t inputSize, void* output, std::size_t maxOutputSize)
          : Delegate()
        {
          SetInput(input, inputSize);
          SetOutput(output, maxOutputSize);
        }
      protected:
        void SetInput(const void* src, std::size_t srcSize)
        {
          Delegate.next_in = const_cast<Bytef*>(static_cast<const Bytef*>(src));
          Delegate.avail_in = static_cast<uInt>(srcSize);
        }
        
        void SetOutput(void* dst, std::size_t dstSize)
        {
          Delegate.next_out = static_cast<Bytef*>(dst);
          Delegate.avail_out = dstSize;
        }
        
        static void CheckError(int code, Error::LocationRef line)
        {
          if (code != Z_OK)
          {
            throw Error(line, zError(code));
          }
        }
      protected:
        z_stream Delegate;
      };
     
      class DecompressStream : public Stream
      {
      public:
        DecompressStream()
          : Stream()
        {
        }
        
        DecompressStream(const void* input, std::size_t inputSize, void* output, std::size_t maxOutputSize)
          : Stream(input, inputSize, output, maxOutputSize)
        {
        }

        ~DecompressStream()
        {
          ::inflateEnd(&Delegate);
        }

        void InitRaw()
        {
          CheckError(::inflateInit2(&Delegate, -15), THIS_LINE);
        }
        
        void Init()
        {
          CheckError(::inflateInit(&Delegate), THIS_LINE);
        }
        
        void Decompress(DataInputStream& input, DataBuilder& output, std::size_t outputSizeHint)
        {
          const auto prevOutSize = output.Size();

          SetInput(input.PeekRawData(0), input.GetRestSize());
          if (outputSizeHint != 0)
          {
            SetOutput(output.Allocate(outputSizeHint), outputSizeHint);
          }
          for (;;)
          {
            if (Delegate.avail_out == 0)
            {
              const auto bufSize = Math::Align<std::size_t>(Delegate.total_in ? uint64_t(Delegate.avail_in) * Delegate.total_out / Delegate.total_in : 1, 16384);
              SetOutput(output.Allocate(bufSize), bufSize);
            }
            const auto res = ::inflate(&Delegate, Z_SYNC_FLUSH);
            if (res == Z_STREAM_END)
            {
              break;
            }
            CheckError(res, THIS_LINE);
          }
          input.Skip(Delegate.total_in);
          output.Resize(prevOutSize + Delegate.total_out);
        }
        
        std::size_t Decompress()
        {
          const auto res = ::inflate(&Delegate, Z_FINISH);
          if (res != Z_OK && res != Z_STREAM_END)
          {
            CheckError(res, THIS_LINE);
          }
          return Delegate.total_out;
        }
      };
      
      void DecompressRaw(DataInputStream& input, DataBuilder& output, std::size_t outputSizeHint)
      {
        DecompressStream stream;
        stream.InitRaw();
        stream.Decompress(input, output, outputSizeHint);
      }

      std::size_t DecompressRaw(const void* input, std::size_t inputSize, void* output, std::size_t maxOutputSize)
      {
        DecompressStream stream(input, inputSize, output, maxOutputSize);
        stream.InitRaw();
        return stream.Decompress();
      }

      void Decompress(DataInputStream& input, DataBuilder& output, std::size_t outputSizeHint)
      {
        DecompressStream stream;
        stream.Init();
        stream.Decompress(input, output, outputSizeHint);
      }
      
      std::size_t Decompress(const void* input, std::size_t inputSize, void* output, std::size_t maxOutputSize)
      {
        DecompressStream stream(input, inputSize, output, maxOutputSize);
        stream.Init();
        return stream.Decompress();
      }
      
      class CompressStream : public Stream
      {
      public:
        using Stream::Stream;

        ~CompressStream()
        {
          ::deflateEnd(&Delegate);
        }
        
        void Init()
        {
          CheckError(::deflateInit(&Delegate, Z_BEST_COMPRESSION), THIS_LINE);
        }
        
        std::size_t Compress()
        {
          const auto res = ::deflate(&Delegate, Z_FINISH);
          if (res != Z_STREAM_END)
          {
            CheckError(res == Z_OK ? Z_BUF_ERROR : res, THIS_LINE);
          }
          return Delegate.total_out;
        }
        
        void Compress(DataInputStream& input, DataBuilder& output)
        {
          const auto inData = input.ReadRestData();
          const auto prevOutSize = output.Size();

          SetInput(inData.Start(), inData.Size());
          const auto approxOutSize = ::compressBound(static_cast<uLong>(inData.Size()));
          SetOutput(output.Allocate(approxOutSize), approxOutSize);
          const auto outSize = Compress();
          output.Resize(prevOutSize + outSize);
        }
      };
      
      void Compress(DataInputStream& input, DataBuilder& output)
      {
        CompressStream stream;
        stream.Init();
        stream.Compress(input, output);
      }
    }
  }
}
