/**
 *
 * @file
 *
 * @brief  HES support implementation
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
#include <binary/container_base.h>
#include <binary/crc.h>
#include <binary/format_factories.h>
#include <formats/multitrack.h>
#include <math/numeric.h>
// std includes
#include <array>
#include <cstring>
#include <utility>

namespace Formats::Multitrack
{
  namespace HES
  {
    using SignatureType = std::array<uint8_t, 4>;

    const SignatureType SIGNATURE = {{'H', 'E', 'S', 'M'}};

    struct RawHeader
    {
      SignatureType Signature;
      uint8_t Version;
      uint8_t StartSong;
      le_uint16_t RequestAddress;
      uint8_t InitialMPR[8];
      SignatureType DataSignature;
      le_uint32_t DataSize;
      le_uint32_t DataAddress;
      le_uint32_t Unused;
      // uint8_t Unused2[0x20];
      // uint8_t Fields[0x90];
    };

    static_assert(sizeof(RawHeader) * alignof(RawHeader) == 0x20, "Invalid layout");

    const auto FORMAT =
        "'H'E'S'M"   // signature
        "?"          // version
        "?"          // start song
        "??"         // requested address
        "?{8}"       // MPR
        "'D'A'T'A"   // data signature
        "? ? 0x 00"  // 1MB size limit
        "? ? 0x 00"  // 1MB size limit
        ""sv;

    const auto DESCRIPTION = "Home Entertainment System"sv;

    const std::size_t MIN_SIZE = 256;

    const uint_t TOTAL_TRACKS_COUNT = 32;

    const RawHeader* GetHeader(Binary::View rawData)
    {
      if (rawData.Size() < MIN_SIZE)
      {
        return nullptr;
      }
      const auto* hdr = safe_ptr_cast<const RawHeader*>(rawData.Start());
      if (hdr->Signature != SIGNATURE)
      {
        return nullptr;
      }
      return hdr;
    }

    class Container : public Binary::BaseContainer<Multitrack::Container>
    {
    public:
      Container(const RawHeader* hdr, Binary::Container::Ptr data)
        : BaseContainer(std::move(data))
        , Hdr(hdr)
      {}

      uint_t FixedChecksum() const override
      {
        return Binary::Crc32(*Delegate);
      }

      uint_t TracksCount() const override
      {
        return TOTAL_TRACKS_COUNT;
      }

      uint_t StartTrackIndex() const override
      {
        return Hdr->StartSong;
      }

    private:
      const RawHeader* const Hdr;
    };

    class Decoder : public Formats::Multitrack::Decoder
    {
    public:
      // Use match only due to lack of end detection
      Decoder()
        : Format(Binary::CreateMatchOnlyFormat(FORMAT, MIN_SIZE))
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

      Formats::Multitrack::Container::Ptr Decode(const Binary::Container& rawData) const override
      {
        if (const auto* hdr = GetHeader(rawData))
        {
          const std::size_t totalSize = sizeof(*hdr) + hdr->DataSize;
          // GME support truncated files
          const std::size_t realSize = std::min(rawData.Size(), totalSize);
          auto used = rawData.GetSubcontainer(0, realSize);
          return MakePtr<Container>(hdr, std::move(used));
        }
        else
        {
          return {};
        }
      }

    private:
      const Binary::Format::Ptr Format;
    };
  }  // namespace HES

  Decoder::Ptr CreateHESDecoder()
  {
    return MakePtr<HES::Decoder>();
  }
}  // namespace Formats::Multitrack
