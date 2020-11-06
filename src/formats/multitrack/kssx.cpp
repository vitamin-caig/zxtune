/**
* 
* @file
*
* @brief  Multitrack KSS support implementation
*
* @author vitamin.caig@gmail.com
*
**/

//common includes
#include <byteorder.h>
#include <contract.h>
#include <pointers.h>
#include <make_ptr.h>
//library includes
#include <binary/container_base.h>
#include <binary/container_factories.h>
#include <binary/crc.h>
#include <binary/format_factories.h>
#include <formats/multitrack.h>
//std includes
#include <array>
#include <cstring>
#include <utility>

namespace Formats
{
namespace Multitrack
{
  namespace KSSX
  {
    typedef std::array<uint8_t, 4> SignatureType;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
    PACK_PRE struct RawHeader
    {
      SignatureType Signature;
      uint16_t LoadAddress;
      uint16_t InitialDataSize;
      uint16_t InitAddress;
      uint16_t PlayAddress;
      uint8_t StartBank;
      uint8_t ExtraBanks;
      uint8_t ExtraHeaderSize;
      uint8_t ExtraChips;
    } PACK_POST;
    
    PACK_PRE struct ExtraHeader
    {
      uint32_t DataSize;
      uint32_t Reserved;
      uint16_t FirstTrack;
      uint16_t LastTrack;
      /* Optional part
      uint8_t PsgVolume;
      uint8_t SccVolume;
      uint8_t MsxMusVolume;
      uint8_t MsxAudVolume;
      */
    } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

    static_assert(sizeof(RawHeader) == 0x10, "Invalid layout");
    static_assert(sizeof(ExtraHeader) == 0x0c, "Invalid layout");

    const StringView FORMAT =
        "'K'S'S'X" //signature
        "??"       //load address
        "??"       //initial data size
        "??"       //init address
        "??"       //play address
        "?"        //start bank
        "?"        //extra banks
        "00|0c-10" //extra header size
        "%0x0xxxxx"//extra chips
     ;
    const ExtraHeader STUB_EXTRA_HEADER = {~uint32_t(0), 0, 0, 0};
     
    const std::size_t MIN_SIZE = sizeof(RawHeader) + sizeof(ExtraHeader);
    
    const uint_t MAX_TRACKS_COUNT = 256;

    class Container : public Binary::BaseContainer<Multitrack::Container>
    {
    public:
      Container(const ExtraHeader* hdr, Binary::Container::Ptr data)
        : BaseContainer(std::move(data))
        , Hdr(hdr)
      {
      }
      
      uint_t FixedChecksum() const override
      {
        const Binary::View data(*Delegate);
        const auto* header = data.As<RawHeader>();
        const std::size_t headersSize = sizeof(*header) + header->ExtraHeaderSize;
        return Binary::Crc32(data.SubView(headersSize));
      }

      uint_t TracksCount() const override
      {
        return fromLE(Hdr->LastTrack) + 1;
      }

      uint_t StartTrackIndex() const override
      {
        return fromLE(Hdr->FirstTrack);
      }
      
      Container::Ptr WithStartTrackIndex(uint_t idx) const override
      {
        Require(Hdr != &STUB_EXTRA_HEADER);
        std::unique_ptr<Dump> content(new Dump(Delegate->Size()));
        std::memcpy(content->data(), Delegate->Start(), content->size());
        ExtraHeader* const hdr = safe_ptr_cast<ExtraHeader*>(content->data() + sizeof(RawHeader));
        Require(idx <= hdr->LastTrack);
        hdr->FirstTrack = idx;
        return MakePtr<Container>(hdr, Binary::CreateContainer(std::move(content)));
      }
    private:
      const ExtraHeader* const Hdr;
    };
     
    class Decoder : public Formats::Multitrack::Decoder
    {
    public:
      Decoder()
        : Format(Binary::CreateFormat(FORMAT, MIN_SIZE))
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
        if (!Format->Match(rawData))
        {
          return Formats::Multitrack::Container::Ptr();
        }
        const std::size_t availSize = rawData.Size();
        const RawHeader* const hdr = safe_ptr_cast<const RawHeader*>(rawData.Start());
        const ExtraHeader* const extraHdr = hdr->ExtraHeaderSize != 0 ? safe_ptr_cast<const ExtraHeader*>(hdr + 1) : &STUB_EXTRA_HEADER;
        if (fromLE(extraHdr->LastTrack) > MAX_TRACKS_COUNT - 1 || extraHdr->Reserved != 0)
        {
          return Formats::Multitrack::Container::Ptr();
        }
        const std::size_t headersSize = sizeof(*hdr) + hdr->ExtraHeaderSize;
        const std::size_t bankSize = 0 != (hdr->ExtraBanks & 0x80) ? 8192 : 16384;
        const uint_t banksCount = hdr->ExtraBanks & 0x7f;
        const std::size_t totalSize = headersSize + fromLE(hdr->InitialDataSize) + bankSize * banksCount;
        //GME support truncated files
        const Binary::Container::Ptr used = rawData.GetSubcontainer(0, std::min(availSize, totalSize));
        return MakePtr<Container>(extraHdr, used);
      }
    private:
      const Binary::Format::Ptr Format;
    };
  }

  Decoder::Ptr CreateKSSXDecoder()
  {
    return MakePtr<KSSX::Decoder>();
  }
}
}
