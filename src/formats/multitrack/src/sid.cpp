/**
 *
 * @file
 *
 * @brief  SID support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/container.h"

#include "binary/container_base.h"
#include "binary/format_factories.h"
#include "formats/multitrack/decoder.h"
#include "math/numeric.h"

#include "byteorder.h"
#include "contract.h"
#include "make_ptr.h"
#include "pointers.h"

#include <array>
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
        ""sv;

    const auto DESCRIPTION = "Commodore64 SID/RSID/PSID"sv;

    class Container : public Binary::BaseContainer<Multitrack::Container, Chiptune::Container>
    {
    public:
      Container(const RawHeader* hdr, const Binary::Container& data)
        : BaseContainer(Chiptune::CreateCalculatingCrcContainer(data))
        , Hdr(hdr)
      {}

      uint_t Checksum() const override
      {
        return Delegate->Checksum();
      }

      uint_t FixedChecksum() const override
      {
        return Delegate->FixedChecksum();
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

    class Decoder : public Multitrack::Decoder
    {
    public:
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
        return Format->Match(rawData) && GetHeader(rawData);
      }

      Container::Ptr Decode(const Binary::Container& rawData) const override
      {
        if (const auto* hdr = GetHeader(rawData))
        {
          return MakePtr<Container>(hdr, rawData);
        }
        else
        {
          return {};
        }
      }

    private:
      const Binary::Format::Ptr Format = Binary::CreateMatchOnlyFormat(FORMAT);
    };
  }  // namespace SID

  Decoder::Ptr CreateSIDDecoder()
  {
    return MakePtr<SID::Decoder>();
  }
}  // namespace Formats::Multitrack
