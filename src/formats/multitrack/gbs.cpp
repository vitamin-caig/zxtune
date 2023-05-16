/**
 *
 * @file
 *
 * @brief  GBS support implementation
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
  namespace GBS
  {
    typedef std::array<uint8_t, 3> SignatureType;

    typedef std::array<char, 32> StringType;

    const SignatureType SIGNATURE = {{'G', 'B', 'S'}};

    struct RawHeader
    {
      SignatureType Signature;
      uint8_t Version;
      uint8_t SongsCount;
      uint8_t StartSong;  // 1-based
      le_uint16_t LoadAddr;
      le_uint16_t InitAddr;
      le_uint16_t PlayAddr;
      le_uint16_t StackPointer;
      uint8_t TimerModulo;
      uint8_t TimerControl;
      StringType Title;
      StringType Artist;
      StringType Copyright;
    };

    static_assert(sizeof(RawHeader) * alignof(RawHeader) == 112, "Invalid layout");

    const std::size_t MAX_SIZE = 1048576;

    const auto FORMAT =
        "'G'B'S"
        "01"     // version
        "01-ff"  // 1 song minimum
        "01-ff"  // first song
        // do not pay attention to addresses
        ""_sv;

    const Char DESCRIPTION[] = "GameBoy Sound";

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
        const uint32_t part2 = Binary::Crc32(data.SubView(sizeof(RawHeader)), part1);
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
  }  // namespace GBS

  Decoder::Ptr CreateGBSDecoder()
  {
    return MakePtr<GBS::Decoder>();
  }
}  // namespace Formats::Multitrack
