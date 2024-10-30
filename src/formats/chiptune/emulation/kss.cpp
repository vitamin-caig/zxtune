/**
 *
 * @file
 *
 * @brief  KSS support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// common includes
#include <byteorder.h>
#include <contract.h>
#include <make_ptr.h>
#include <pointers.h>
// library includes
#include <binary/format_factories.h>
#include <formats/chiptune/container.h>
#include <math/numeric.h>
// std includes
#include <array>
#include <cstring>

namespace Formats::Chiptune
{
  namespace KSS
  {
    const auto DESCRIPTION = "KSS Music Format"sv;

    using SignatureType = std::array<uint8_t, 4>;

    struct RawHeader
    {
      SignatureType Signature;
      le_uint16_t LoadAddress;
      le_uint16_t InitialDataSize;
      le_uint16_t InitAddress;
      le_uint16_t PlayAddress;
      uint8_t StartBank;
      uint8_t ExtraBanks;
      uint8_t ExtraHeaderSize;
      uint8_t ExtraChips;
    };

    static_assert(sizeof(RawHeader) * alignof(RawHeader) == 0x10, "Invalid layout");

    const auto FORMAT =
        "'K'S'C'C"   // signature
        "??"         // load address
        "??"         // initial data size
        "??"         // init address
        "??"         // play address
        "?"          // start bank
        "?"          // extra banks
        "00"         // reserved
        "%000xxxxx"  // extra chips (some of the tunes has 4th bit set)
        ""sv;

    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      Decoder()
        : Format(Binary::CreateFormat(FORMAT))
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

      Formats::Chiptune::Container::Ptr Decode(const Binary::Container& rawData) const override
      {
        if (!Format->Match(rawData))
        {
          return {};
        }
        const RawHeader& hdr = *safe_ptr_cast<const RawHeader*>(rawData.Start());
        const std::size_t bankSize = 0 != (hdr.ExtraBanks & 0x80) ? 8192 : 16384;
        const uint_t banksCount = hdr.ExtraBanks & 0x7f;
        const std::size_t totalSize = sizeof(hdr) + hdr.InitialDataSize + bankSize * banksCount;
        // GME support truncated files
        const std::size_t realSize = std::min(rawData.Size(), totalSize);
        const Binary::Container::Ptr data = rawData.GetSubcontainer(0, realSize);
        return CreateCalculatingCrcContainer(data, 0, realSize);
      }

    private:
      const Binary::Format::Ptr Format;
    };
  }  // namespace KSS

  Decoder::Ptr CreateKSSDecoder()
  {
    return MakePtr<KSS::Decoder>();
  }
}  // namespace Formats::Chiptune
