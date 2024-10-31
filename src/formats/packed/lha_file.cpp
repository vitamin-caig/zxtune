/**
 *
 * @file
 *
 * @brief  LHA compressor support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/packed/container.h"
#include "formats/packed/lha_supp.h"

#include "binary/data_builder.h"
#include "binary/input_stream.h"
#include "debug/log.h"
#include "formats/packed.h"

#include "make_ptr.h"
#include "string_type.h"
#include "string_view.h"
#include "types.h"

#include "3rdparty/lhasa/lib/lha_decoder.h"

#include <memory>

namespace Formats::Packed::Lha
{
  const Debug::Stream Dbg("Formats::Packed::Lha");

  class Decompressor
  {
  public:
    using Ptr = std::shared_ptr<const Decompressor>;
    virtual ~Decompressor() = default;

    virtual Formats::Packed::Container::Ptr Decode(const Binary::Container& rawData, std::size_t outputSize) const = 0;
  };

  class LHADecompressor : public Decompressor
  {
  public:
    explicit LHADecompressor(LHADecoderType* type)
      : Type(type)
    {}

    Formats::Packed::Container::Ptr Decode(const Binary::Container& rawData, std::size_t outputSize) const override
    {
      Binary::InputStream input(rawData);
      const std::shared_ptr<LHADecoder> decoder(::lha_decoder_new(Type, &ReadData, &input, outputSize),
                                                &::lha_decoder_free);
      Binary::DataBuilder result;
      if (const std::size_t decoded =
              ::lha_decoder_read(decoder.get(), static_cast<uint8_t*>(result.Allocate(outputSize)), outputSize))
      {
        const std::size_t originalSize = input.GetPosition();
        Dbg("Decoded {} -> {} bytes", originalSize, outputSize);
        return CreateContainer(result.CaptureResult(), originalSize);
      }
      return {};
    }

  private:
    static size_t ReadData(void* buf, size_t len, void* data)
    {
      return static_cast<Binary::InputStream*>(data)->Read(buf, len);
    }

  private:
    LHADecoderType* const Type;
  };

  // TODO: create own decoders for non-packing modes
  Decompressor::Ptr CreateDecompressor(const String& method)
  {
    if (LHADecoderType* type = ::lha_decoder_for_name(const_cast<char*>(method.c_str())))
    {
      return MakePtr<LHADecompressor>(type);
    }
    return {};
  }

  Formats::Packed::Container::Ptr DecodeRawData(const Binary::Container& input, const String& method,
                                                std::size_t outputSize)
  {
    if (Formats::Packed::Container::Ptr result = DecodeRawDataAtLeast(input, method, outputSize))
    {
      const std::size_t decoded = result->Size();
      if (decoded == outputSize)
      {
        return result;
      }
      const std::size_t originalSize = result->PackedSize();
      Dbg("Output size mismatch while decoding {} -> {} ({} required)", originalSize, decoded, outputSize);
    }
    return {};
  }

  Formats::Packed::Container::Ptr DecodeRawDataAtLeast(const Binary::Container& input, const String& method,
                                                       std::size_t sizeHint)
  {
    if (const Lha::Decompressor::Ptr decompressor = Lha::CreateDecompressor(method))
    {
      return decompressor->Decode(input, sizeHint);
    }
    return {};
  }
}  // namespace Formats::Packed::Lha
