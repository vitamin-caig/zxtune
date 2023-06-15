/**
 *
 * @file
 *
 * @brief  SID support implementation
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
#include <utility>

namespace Formats::Multitrack
{
  namespace SID
  {
    using SignatureType = std::array<uint8_t, 4>;

    const SignatureType SIGNATURE_RSID = {{'R', 'S', 'I', 'D'}};
    const SignatureType SIGNATURE_PSID = {{'P', 'S', 'I', 'D'}};

    const uint_t VERSION_MIN = 1;
    const uint_t VERSION_MAX = 3;

    struct RawHeader
    {
      SignatureType Signature;
      be_uint16_t Version;
      be_uint16_t DataOffset;
      be_uint16_t LoadAddr;
      be_uint16_t InitAddr;
      be_uint16_t PlayAddr;
      be_uint16_t SongsCount;
      be_uint16_t StartSong;
      be_uint32_t SpeedFlags;
    };

    static_assert(sizeof(RawHeader) * alignof(RawHeader) == 22, "Invalid layout");

    const auto FORMAT =
        "'R|'P 'S'I'D"  // signature
        "00 01-03"      // BE version
        "00 76|7c"      // BE data offset
        "??"            // BE load address
        "??"            // BE init address
        "??"            // BE play address
        "00|01 ?"       // BE songs count 1-256
        "??"            // BE start song
        "????"          // BE speed flag
        ""_sv;

    const Char DESCRIPTION[] = "Commodore64 SID/RSID/PSID";

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
        return Hdr->SongsCount;
      }

      uint_t StartTrackIndex() const override
      {
        return Hdr->StartSong - 1;
      }

    private:
      const RawHeader* const Hdr;
    };

    const RawHeader* GetHeader(Binary::View rawData)
    {
      if (rawData.Size() < sizeof(RawHeader))
      {
        return nullptr;
      }
      const auto* hdr = safe_ptr_cast<const RawHeader*>(rawData.Start());
      if (hdr->Signature != SIGNATURE_PSID && hdr->Signature != SIGNATURE_RSID)
      {
        return nullptr;
      }
      if (!Math::InRange<uint_t>(hdr->Version, VERSION_MIN, VERSION_MAX))
      {
        return nullptr;
      }
      return hdr;
    }

    class Decoder : public Formats::Multitrack::Decoder
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
        return Format->Match(rawData) && GetHeader(rawData);
      }

      Container::Ptr Decode(const Binary::Container& rawData) const override
      {
        if (const auto* hdr = GetHeader(rawData))
        {
          auto used = rawData.GetSubcontainer(0, rawData.Size());
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
  }  // namespace SID

  Decoder::Ptr CreateSIDDecoder()
  {
    return MakePtr<SID::Decoder>();
  }
}  // namespace Formats::Multitrack
