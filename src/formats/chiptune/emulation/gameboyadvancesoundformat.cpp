/**
 *
 * @file
 *
 * @brief  GSF parser implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "formats/chiptune/emulation/gameboyadvancesoundformat.h"
// common includes
#include <byteorder.h>
#include <make_ptr.h>
// library includes
#include <binary/format_factories.h>
#include <binary/input_stream.h>

namespace Formats::Chiptune
{
  namespace GameBoyAdvanceSoundFormat
  {
    const Char DESCRIPTION[] = "GameBoy Advance Sound Format";

    /*
    Offset         Size    Description
    0x0000000      4       GSF_Entry_Point
    0x0000004      4       GSF_Offset
    0x0000008      4       Size of Rom.
    0x000000C      XX      The Rom data itself.
    */
    void ParseRom(Binary::View data, Builder& target)
    {
      Binary::DataInputStream stream(data);
      target.SetEntryPoint(stream.Read<le_uint32_t>());
      const auto addr = stream.Read<le_uint32_t>();
      const std::size_t size = stream.Read<le_uint32_t>();
      const std::size_t avail = stream.GetRestSize();
      target.SetRom(addr, stream.ReadData(std::min(size, avail)));
    }

    const auto FORMAT =
        "'P'S'F"
        "22"
        ""sv;

    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      Decoder()
        : Format(Binary::CreateMatchOnlyFormat(FORMAT))
      {}

      String GetDescription() const override
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
  }  // namespace GameBoyAdvanceSoundFormat

  Decoder::Ptr CreateGSFDecoder()
  {
    return MakePtr<GameBoyAdvanceSoundFormat::Decoder>();
  }
}  // namespace Formats::Chiptune
