/**
 *
 * @file
 *
 * @brief  Compress/decompress functions implementation based on zlib
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "binary/compression/zlib.h"

#include "binary/data_builder.h"
#include "binary/input_stream.h"
#include "math/numeric.h"

#include "error.h"

#include "3rdparty/zlib/zlib.h"

namespace Binary::Compression::Zlib
{
  void CheckError(int code, Error::LocationRef line)
  {
    if (code != Z_OK)
    {
      throw Error(line, "Zlib: "s + zError(code));
    }
  }

  class Inflate
  {
    enum Mode : int
    {
      ZLIB = 15,
      RAW = -15
    };

    Inflate(Binary::View input, Mode mode)
    {
      Delegate.next_in = const_cast<Bytef*>(static_cast<const Bytef*>(input.Start()));
      Delegate.avail_in = static_cast<uInt>(input.Size());
      CheckError(::inflateInit2(&Delegate, mode), THIS_LINE);
    }

  public:
    ~Inflate()
    {
      ::inflateEnd(&Delegate);
    }

    std::size_t To(DataBuilder& output, std::size_t outputSizeHint = 0)
    {
      if (outputSizeHint != 0)
      {
        SetOutput(output.Allocate(outputSizeHint), outputSizeHint);
      }
      for (;;)
      {
        if (Delegate.avail_out == 0)
        {
          const auto restInput = uint64_t(Delegate.avail_in);
          const auto forecastOutput = Delegate.total_in ? restInput * Delegate.total_out / Delegate.total_in
                                                        : restInput * 2;
          const auto bufSize = Math::Align<std::size_t>(forecastOutput, 16384);
          SetOutput(output.Allocate(bufSize), bufSize);
        }
        const auto res = ::inflate(&Delegate, Z_SYNC_FLUSH);
        if (res == Z_STREAM_END)
        {
          break;
        }
        CheckError(res, THIS_LINE);
      }
      output.Resize(output.Size() - Delegate.avail_out);
      if (outputSizeHint != 0 && Delegate.total_out != outputSizeHint)
      {
        throw Error(THIS_LINE, "Zlib: decompressed size mismatch");
      }
      return Delegate.total_in;
    }

    static std::size_t Zlib(Binary::View input, DataBuilder& output, std::size_t outputSizeHint = 0)
    {
      return Inflate(input, Mode::ZLIB).To(output, outputSizeHint);
    }

    static std::size_t Raw(Binary::View input, DataBuilder& output, std::size_t outputSizeHint = 0)
    {
      return Inflate(input, Mode::RAW).To(output, outputSizeHint);
    }

  private:
    void SetOutput(void* dst, std::size_t dstSize)
    {
      Delegate.next_out = static_cast<Bytef*>(dst);
      Delegate.avail_out = dstSize;
    }

  private:
    z_stream Delegate = {};
  };

  std::size_t Compress(View input, DataBuilder& output)
  {
    const auto prevOutSize = output.Size();
    const auto inSize = static_cast<uLong>(input.Size());
    auto outSize = ::compressBound(inSize);
    CheckError(::compress2(static_cast<Byte*>(output.Allocate(outSize)), &outSize, input.As<Byte>(), inSize,
                           Z_BEST_COMPRESSION),
               THIS_LINE);
    output.Resize(prevOutSize + outSize);
    return outSize;
  }

  Container::Ptr Decompress(View packed, std::size_t unpackedSize)
  {
    DataBuilder out(unpackedSize);
    Inflate::Zlib(packed, out, unpackedSize);
    return out.CaptureResult();
  }

  void Decompress(View input, DataBuilder& output)
  {
    Inflate::Zlib(input, output);
  }

  Container::Ptr DecompressRaw(View packed, std::size_t unpackedSize)
  {
    DataBuilder out(unpackedSize);
    Inflate::Raw(packed, out, unpackedSize);
    return out.CaptureResult();
  }

  Container::Ptr DecompressRaw(DataInputStream& input)
  {
    DataBuilder out;
    const auto consumed = Inflate::Raw({input.PeekRawData(0), input.GetRestSize()}, out);
    input.Skip(consumed);
    return out.CaptureResult();
  }

  Container::Ptr Compress(View input)
  {
    DataBuilder out;
    Compress(input, out);
    return out.CaptureResult();
  }
}  // namespace Binary::Compression::Zlib
