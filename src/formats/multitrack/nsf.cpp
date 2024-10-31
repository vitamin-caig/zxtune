/**
 *
 * @file
 *
 * @brief  NSF support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include <binary/container_base.h>
#include <binary/crc.h>
#include <binary/format_factories.h>
#include <formats/multitrack.h>
#include <math/numeric.h>

#include <byteorder.h>
#include <contract.h>
#include <make_ptr.h>
#include <pointers.h>

#include <array>
#include <cstring>
#include <utility>

namespace Formats::Multitrack
{
  namespace NSF
  {
    using SignatureType = std::array<uint8_t, 5>;

    using StringType = std::array<char, 32>;

    const SignatureType SIGNATURE = {{'N', 'E', 'S', 'M', '\x1a'}};

    struct RawHeader
    {
      SignatureType Signature;
      uint8_t Version;
      uint8_t SongsCount;
      uint8_t StartSong;  // 1-based
      le_uint16_t LoadAddr;
      le_uint16_t InitAddr;
      le_uint16_t PlayAddr;
      StringType Title;
      StringType Artist;
      StringType Copyright;
      le_uint16_t NTSCSpeedUs;
      uint8_t Bankswitch[8];
      le_uint16_t PALSpeedUs;
      uint8_t Mode;
      uint8_t ExtraDevices;
      uint8_t Expansion[4];
    };

    static_assert(sizeof(RawHeader) * alignof(RawHeader) == 128, "Invalid layout");

    const std::size_t MAX_SIZE = 1048576;

    const auto FORMAT =
        "'N'E'S'M"
        "1a"
        "?"      // version
        "01-ff"  // 1 song minimum
        "01-ff"
        // gme supports nfs load/init address starting from 0x8000 or zero
        "(? 80-ff){2}"
        ""sv;

    const auto DESCRIPTION = "NES Sound Format"sv;

    const std::size_t MIN_SIZE = 256;

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
      if (!hdr->SongsCount || !hdr->StartSong)
      {
        return nullptr;
      }
      if (hdr->LoadAddr < 0x8000 || hdr->InitAddr < 0x8000)
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
        // just skip text fields
        const Binary::View data(*Delegate);
        const uint32_t part1 = Binary::Crc32(data.SubView(0, offsetof(RawHeader, Title)));
        const uint32_t part2 = Binary::Crc32(data.SubView(offsetof(RawHeader, NTSCSpeedUs)), part1);
        return part2;
      }

      uint_t TracksCount() const override
      {
        return Hdr->SongsCount;
      }

      uint_t StartTrackIndex() const override
      {
        return Hdr->StartSong - 1;
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
          auto used = rawData.GetSubcontainer(0, std::min(rawData.Size(), MAX_SIZE));
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
  }  // namespace NSF

  Decoder::Ptr CreateNSFDecoder()
  {
    return MakePtr<NSF::Decoder>();
  }
}  // namespace Formats::Multitrack
