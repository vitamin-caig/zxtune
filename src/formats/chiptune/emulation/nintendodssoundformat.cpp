/**
 *
 * @file
 *
 * @brief  2SF parser implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/emulation/nintendodssoundformat.h"

#include "binary/compression/zlib_container.h"
#include "binary/format.h"
#include "binary/format_factories.h"
#include "binary/input_stream.h"

#include "byteorder.h"
#include "contract.h"
#include "make_ptr.h"
#include "string_view.h"

#include <algorithm>
#include <array>
#include <memory>

namespace Formats::Chiptune
{
  namespace NintendoDSSoundFormat
  {
    const auto DESCRIPTION = "Nintendo DS Sound Format"sv;

    using SignatureType = std::array<uint8_t, 4>;
    const SignatureType SAVESTATE_SIGNATURE = {{'S', 'A', 'V', 'E'}};

    void ParseRom(Binary::View data, Builder& target)
    {
      Binary::DataInputStream stream(data);
      const uint32_t offset = stream.Read<le_uint32_t>();
      const std::size_t size = stream.Read<le_uint32_t>();
      target.SetChunk(offset, stream.ReadData(size));
      Require(0 == stream.GetRestSize());
    }

    void ParseState(Binary::View data, Builder& target)
    {
      Binary::DataInputStream stream(data);
      while (stream.GetRestSize() >= sizeof(SignatureType) + sizeof(uint32_t) + sizeof(uint32_t))
      {
        const auto signature = stream.Read<SignatureType>();
        const std::size_t packedSize = stream.Read<le_uint32_t>();
        /*const auto unpackedCrc = */ stream.Read<le_uint32_t>();
        if (signature == SAVESTATE_SIGNATURE)
        {
          auto packedData = stream.ReadData(packedSize);
          const auto unpackedPart = Binary::Compression::Zlib::Decompress(packedData);
          // do not check crc32
          ParseRom(*unpackedPart, target);
        }
        else
        {
          // just try to skip as much as possible
          stream.Skip(std::min<std::size_t>(stream.GetRestSize(), packedSize));
        }
      }
    }

    const auto FORMAT =
        "'P'S'F"
        "24"
        ""sv;

    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      Decoder()
        : Format(Binary::CreateMatchOnlyFormat(FORMAT))
      {}

      StringView GetDescription() const override
      {
        return DESCRIPTION;
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
        return {};  // TODO
      }

    private:
      const Binary::Format::Ptr Format;
    };
  }  // namespace NintendoDSSoundFormat

  Decoder::Ptr Create2SFDecoder()
  {
    return MakePtr<NintendoDSSoundFormat::Decoder>();
  }
}  // namespace Formats::Chiptune
