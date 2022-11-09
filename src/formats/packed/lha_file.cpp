/**
 *
 * @file
 *
 * @brief  LHA compressor support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "formats/packed/container.h"
#include "formats/packed/lha_supp.h"
#include "formats/packed/pack_utils.h"
// common includes
#include <make_ptr.h>
// library includes
#include <binary/input_stream.h>
#include <debug/log.h>
#include <formats/archived.h>
// 3rdparty includes
#include <3rdparty/lhasa/lib/lha_decoder.h>

namespace Formats::Packed::Lha
{
  const Debug::Stream Dbg("Formats::Packed::Lha");

  class Decompressor
  {
  public:
    typedef std::shared_ptr<const Decompressor> Ptr;
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
      std::unique_ptr<Binary::Dump> result(new Binary::Dump(outputSize));
      if (const std::size_t decoded = ::lha_decoder_read(decoder.get(), result->data(), outputSize))
      {
        const std::size_t originalSize = input.GetPosition();
        Dbg("Decoded {} -> {} bytes", originalSize, outputSize);
        return CreateContainer(std::move(result), originalSize);
      }
      return Formats::Packed::Container::Ptr();
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
    return Decompressor::Ptr();
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
    return Formats::Packed::Container::Ptr();
  }

  Formats::Packed::Container::Ptr DecodeRawDataAtLeast(const Binary::Container& input, const String& method,
                                                       std::size_t sizeHint)
  {
    if (const Lha::Decompressor::Ptr decompressor = Lha::CreateDecompressor(method))
    {
      return decompressor->Decode(input, sizeHint);
    }
    return Formats::Packed::Container::Ptr();
  }
}  // namespace Formats::Packed::Lha
