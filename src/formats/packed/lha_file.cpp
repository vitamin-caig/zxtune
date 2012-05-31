/*
Abstract:
  Lha archives support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "container.h"
#include "lha_supp.h"
#include "pack_utils.h"
//common includes
#include <logging.h>
#include <tools.h>
//library includes
#include <binary/input_stream.h>
#include <formats/archived.h>
//3rdparty includes
#include <3rdparty/lhasa/lib/lha_decoder.h>
//boost includes
#include <boost/make_shared.hpp>

namespace
{
  const std::string THIS_MODULE("Formats::Packed::Lha");
}

namespace Lha
{
  class Decompressor
  {
  public:
    typedef boost::shared_ptr<const Decompressor> Ptr;
    virtual ~Decompressor() {}

    virtual Formats::Packed::Container::Ptr Decode(const Binary::Container& rawData, std::size_t outputSize) const = 0;
  };

  class LHADecompressor : public Decompressor
  {
  public:
    explicit LHADecompressor(LHADecoderType* type)
      : Type(type)
    {
    }

    virtual Formats::Packed::Container::Ptr Decode(const Binary::Container& rawData, std::size_t outputSize) const
    {
      Binary::InputStream input(rawData);
      const boost::shared_ptr<LHADecoder> decoder(::lha_decoder_new(Type, &ReadData, &input, outputSize), &::lha_decoder_free);
      std::auto_ptr<Dump> result(new Dump(outputSize));
      if (const std::size_t decoded = ::lha_decoder_read(decoder.get(), &result->front(), outputSize))
      {
        const std::size_t originalSize = input.GetPosition();
        Log::Debug(THIS_MODULE, "Decoded %1% -> %2% bytes", originalSize, outputSize);
        return CreatePackedContainer(result, originalSize);
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

  //TODO: create own decoders for non-packing modes
  Decompressor::Ptr CreateDecompressor(const std::string& method)
  {
    if (LHADecoderType* type = ::lha_decoder_for_name(const_cast<char*>(method.c_str())))
    {
      return boost::make_shared<LHADecompressor>(type);
    }
    return Decompressor::Ptr();
  }
}

namespace Formats
{
  namespace Packed
  {
    namespace Lha
    {
      Formats::Packed::Container::Ptr DecodeRawData(const Binary::Container& input, const std::string& method, std::size_t outputSize)
      {
        if (Formats::Packed::Container::Ptr result = DecodeRawDataAtLeast(input, method, outputSize))
        {
          const std::size_t decoded = result->Size();
          if (decoded == outputSize)
          {
            return result;
          }
          const std::size_t originalSize = result->PackedSize();
          Log::Debug(THIS_MODULE, "Output size mismatch while decoding %1% -> %2% (%3% required)", originalSize, decoded, outputSize);
        }
        return Formats::Packed::Container::Ptr();
      }

      Formats::Packed::Container::Ptr DecodeRawDataAtLeast(const Binary::Container& input, const std::string& method, std::size_t sizeHint)
      {
        if (const ::Lha::Decompressor::Ptr decompressor = ::Lha::CreateDecompressor(method))
        {
          return decompressor->Decode(input, sizeHint);
        }
        return Formats::Packed::Container::Ptr();
      }
    }
  }
}

