/**
 *
 * @file
 *
 * @brief  LHA compressor support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "binary/compression/lha.h"

#include "binary/data_builder.h"
#include "binary/input_stream.h"
#include "debug/log.h"

#include "3rdparty/lhasa/lib/lha_decoder.h"

namespace Binary::Compression::Lha
{
  const Debug::Stream Dbg("Binary::Compression::Lha");

  size_t ReadData(void* buf, size_t len, void* data)
  {
    return static_cast<InputStream*>(data)->Read(buf, len);
  }

  Container::Ptr Decode(InputStream& input, LHADecoderType* type, std::size_t maxOutputSize)
  {
    const std::shared_ptr<LHADecoder> decoder(::lha_decoder_new(type, &ReadData, &input, maxOutputSize),
                                              &::lha_decoder_free);
    DataBuilder result(maxOutputSize);
    const auto prevAvail = input.GetRestSize();
    if (const auto decoded =
            ::lha_decoder_read(decoder.get(), static_cast<uint8_t*>(result.Allocate(maxOutputSize)), maxOutputSize))
    {
      result.Resize(decoded);
      const auto avail = input.GetRestSize();
      Dbg("Decoded {}/{} -> {}/{} bytes", prevAvail - avail, prevAvail, decoded, maxOutputSize);
      if (avail != 0 && decoded != maxOutputSize)
      {
        Dbg("Corrupted stream");
        return {};
      }
      return result.CaptureResult();
    }
    return {};
  }

  Container::Ptr DecodeRawData(InputStream& input, const String& method, std::size_t maxOutputSize)
  {
    if (auto* type = ::lha_decoder_for_name(const_cast<char*>(method.c_str())))
    {
      return Decode(input, type, maxOutputSize);
    }
    return {};
  }

  Container::Ptr DecodeRawData(const Container& input, const String& method, std::size_t maxOutputSize)
  {
    InputStream stream(input);
    return DecodeRawData(stream, method, maxOutputSize);
  }
}  // namespace Binary::Compression::Lha
