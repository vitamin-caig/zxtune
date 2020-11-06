/**
* 
* @file
*
* @brief  2SF parser implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "formats/chiptune/emulation/nintendodssoundformat.h"
//common includes
#include <byteorder.h>
#include <make_ptr.h>
//library includes
#include <binary/format_factories.h>
#include <binary/input_stream.h>
#include <binary/compression/zlib_container.h>
//text includes
#include <formats/text/chiptune.h>

namespace Formats
{
namespace Chiptune
{
  namespace NintendoDSSoundFormat
  {
    typedef std::array<uint8_t, 4> SignatureType;
    const SignatureType SAVESTATE_SIGNATURE = {{'S', 'A', 'V', 'E'}};
  
    void ParseRom(Binary::View data, Builder& target)
    {
      Binary::DataInputStream stream(data);
      const auto offset = stream.ReadLE<uint32_t>();
      const auto size = stream.ReadLE<uint32_t>();
      target.SetChunk(offset, stream.ReadData(size));
      Require(0 == stream.GetRestSize());
    }
    
    void ParseState(Binary::View data, Builder& target)
    {
      Binary::DataInputStream stream(data);
      while (stream.GetRestSize() >= sizeof(SignatureType) + sizeof(uint32_t) + sizeof(uint32_t))
      {
        const auto signature = stream.ReadField<SignatureType>();
        const auto packedSize = stream.ReadLE<uint32_t>();
        /*const auto unpackedCrc = */stream.ReadLE<uint32_t>();
        if (signature == SAVESTATE_SIGNATURE)
        {
          auto packedData = stream.ReadData(packedSize);
          const auto unpackedPart = Binary::Compression::Zlib::Decompress(packedData);
          //do not check crc32
          ParseRom(*unpackedPart, target);
        }
        else
        {
          //just try to skip as much as possible
          stream.Skip(std::min<std::size_t>(stream.GetRestSize(), packedSize));
        }
      }
    }
    
    const StringView FORMAT(
      "'P'S'F"
      "24"
    );
    
    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      Decoder()
        : Format(Binary::CreateMatchOnlyFormat(FORMAT))
      {
      }

      String GetDescription() const override
      {
        return Text::NINTENDODSSOUNDFORMAT_DECODER_DESCRIPTION;
      }

      Binary::Format::Ptr GetFormat() const override
      {
        return Format;
      }

      bool Check(Binary::View rawData) const override
      {
        return Format->Match(rawData);
      }

      Formats::Chiptune::Container::Ptr Decode(const Binary::Container& /*rawData*/) const override
      {
        return Formats::Chiptune::Container::Ptr();//TODO
      }
    private:
      const Binary::Format::Ptr Format;
    };
  }

  Decoder::Ptr Create2SFDecoder()
  {
    return MakePtr<NintendoDSSoundFormat::Decoder>();
  }
}
}
