/**
* 
* @file
*
* @brief  NSF support implementation
*
* @author vitamin.caig@gmail.com
*
**/

//common includes
#include <byteorder.h>
#include <contract.h>
#include <make_ptr.h>
#include <pointers.h>
//library includes
#include <binary/container_base.h>
#include <binary/container_factories.h>
#include <binary/crc.h>
#include <binary/format_factories.h>
#include <formats/multitrack.h>
#include <math/numeric.h>
//std includes
#include <array>
#include <cstring>
#include <utility>

namespace Formats
{
namespace Multitrack
{
  namespace NSF
  {
    typedef std::array<uint8_t, 5> SignatureType;
    
    typedef std::array<char, 32> StringType;

    const SignatureType SIGNATURE = {{'N', 'E', 'S', 'M', '\x1a'}};

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
    PACK_PRE struct RawHeader
    {
      SignatureType Signature;
      uint8_t Version;
      uint8_t SongsCount;
      uint8_t StartSong; //1-based
      uint16_t LoadAddr;
      uint16_t InitAddr;
      uint16_t PlayAddr;
      StringType Title;
      StringType Artist;
      StringType Copyright;
      uint16_t NTSCSpeedUs;
      uint8_t Bankswitch[8];
      uint16_t PALSpeedUs;
      uint8_t Mode;
      uint8_t ExtraDevices;
      uint8_t Expansion[4];
    } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

    static_assert(sizeof(RawHeader) == 128, "Invalid layout");
    
    const std::size_t MAX_SIZE = 1048576;

    const StringView FORMAT =
      "'N'E'S'M"
      "1a"
      "?"     //version
      "01-ff" //1 song minimum
      "01-ff"
      //gme supports nfs load/init address starting from 0x8000 or zero
      "(? 80-ff){2}"
     ;
     
    const std::size_t MIN_SIZE = 256;

    const RawHeader* GetHeader(Binary::View rawData)
    {
      if (rawData.Size() < MIN_SIZE)
      {
        return nullptr;
      }
      const RawHeader* hdr = safe_ptr_cast<const RawHeader*>(rawData.Start());
      if (hdr->Signature != SIGNATURE)
      {
        return nullptr;
      }
      if (!hdr->SongsCount || !hdr->StartSong)
      {
        return nullptr;
      }
      if (fromLE(hdr->LoadAddr) < 0x8000 || fromLE(hdr->InitAddr) < 0x8000)
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
      {
      }
      
      uint_t FixedChecksum() const override
      {
        //just skip text fields
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
      
      Container::Ptr WithStartTrackIndex(uint_t idx) const override
      {
        std::unique_ptr<Dump> content(new Dump(Delegate->Size()));
        std::memcpy(content->data(), Delegate->Start(), content->size());
        RawHeader* const hdr = safe_ptr_cast<RawHeader*>(content->data());
        Require(idx < hdr->SongsCount);
        hdr->StartSong = idx + 1;
        return MakePtr<Container>(hdr, Binary::CreateContainer(std::move(content)));
      }
    private:
      const RawHeader* const Hdr;
    };

    class Decoder : public Formats::Multitrack::Decoder
    {
    public:
      //Use match only due to lack of end detection
      Decoder()
        : Format(Binary::CreateMatchOnlyFormat(FORMAT, MIN_SIZE))
      {
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
        if (const RawHeader* hdr = GetHeader(rawData))
        {
          const Binary::Container::Ptr used = rawData.GetSubcontainer(0, std::min(rawData.Size(), MAX_SIZE));
          return MakePtr<Container>(hdr, used);
        }
        else
        {
          return Formats::Multitrack::Container::Ptr();
        }
      }
    private:
      const Binary::Format::Ptr Format;
    };
  }//namespace NSF

  Decoder::Ptr CreateNSFDecoder()
  {
    return MakePtr<NSF::Decoder>();
  }
}//namespace Multitrack
}//namespace Formats
