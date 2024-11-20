/**
 *
 * @file
 *
 * @brief  NCSF parser implementation
 *
 * @author liushuyu011@gmail.com
 *
 **/

#include "formats/chiptune/emulation/nitrocomposersoundformat.h"

#include "binary/compression/zlib_container.h"
#include "binary/format_factories.h"
#include "binary/input_stream.h"

#include "byteorder.h"
#include "make_ptr.h"

#include <array>

namespace Formats::Chiptune
{
  namespace NitroComposerSoundFormat
  {
    const auto DESCRIPTION = "Nitro Composer Sound Format";

    using SignatureType = std::array<uint8_t, 4>;
    const SignatureType SAVESTATE_SIGNATURE = {{'S', 'A', 'V', 'E'}};

    void ParseRom(Binary::View data, Builder& target)
    {
      Binary::DataInputStream stream(data);
      stream.Seek(8);
      const std::size_t size = stream.Read<le_uint32_t>();
      stream.Seek(0);
      target.SetChunk(0, stream.ReadData(size));
      Require(0 == stream.GetRestSize());
    }

    uint32_t ParseState(Binary::View data)
    {
      Binary::DataInputStream stream(data);
      if (data.Size() >= sizeof(uint32_t))
      {
        return stream.Read<le_uint32_t>();
      }
      return 0;
    }

    const auto FORMAT =
        "'P'S'F"
        "25"
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
  }  // namespace NitroComposerSoundFormat

  Decoder::Ptr CreateNCSFDecoder()
  {
    return MakePtr<NitroComposerSoundFormat::Decoder>();
  }
}  // namespace Formats::Chiptune
