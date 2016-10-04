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
#include <crc.h>
#include <pointers.h>
#include <make_ptr.h>
//library includes
#include <binary/container_factories.h>
#include <binary/format_factories.h>
#include <formats/multitrack.h>
//std includes
#include <array>
#include <cstring>

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
      uint32_t Unused;
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

    const std::string FORMAT =
        "'K'S'S'X" //signature
        "??"       //load address
        "??"       //initial data size
        "??"       //init address
        "??"       //play address
        "?"        //start bank
        "?"        //extra banks
        "0c-10"    //extra header size
        "?"        //extra chips
     ;
     
    const std::size_t MIN_SIZE = sizeof(RawHeader) + sizeof(ExtraHeader);

    class Container : public Formats::Multitrack::Container
    {
    public:
      Container(const ExtraHeader* hdr, Binary::Container::Ptr data)
        : Hdr(hdr)
        , Delegate(data)
      {
      }
      
      //Binary::Container
      virtual const void* Start() const
      {
        return Delegate->Start();
      }

      virtual std::size_t Size() const
      {
        return Delegate->Size();
      }

      virtual Binary::Container::Ptr GetSubcontainer(std::size_t offset, std::size_t size) const
      {
        return Delegate->GetSubcontainer(offset, size);
      }
      
      //Formats::Multitrack::Container
      virtual uint_t FixedChecksum() const
      {
        const void* const data = Delegate->Start();
        const RawHeader* const header = static_cast<const RawHeader*>(data);
        const std::size_t headersSize = sizeof(*header) + header->ExtraHeaderSize;
        return Crc32(static_cast<const uint8_t*>(data) + headersSize, Delegate->Size() - headersSize);
      }

      virtual uint_t TracksCount() const
      {
        return fromLE(Hdr->LastTrack) + 1;
      }

      virtual uint_t StartTrackIndex() const
      {
        return fromLE(Hdr->FirstTrack);
      }
      
      virtual Container::Ptr WithStartTrackIndex(uint_t idx) const
      {
        std::unique_ptr<Dump> content(new Dump(Delegate->Size()));
        std::memcpy(&content->front(), Delegate->Start(), content->size());
        ExtraHeader* const hdr = safe_ptr_cast<ExtraHeader*>(&content->front() + sizeof(RawHeader));
        Require(idx <= hdr->LastTrack);
        hdr->FirstTrack = idx;
        return MakePtr<Container>(hdr, Binary::CreateContainer(std::move(content)));
      }
    private:
      const ExtraHeader* const Hdr;
      const Binary::Container::Ptr Delegate;
    };
     
    class Decoder : public Formats::Multitrack::Decoder
    {
    public:
      Decoder()
        : Format(Binary::CreateFormat(FORMAT, MIN_SIZE))
      {
      }

      virtual Binary::Format::Ptr GetFormat() const
      {
        return Format;
      }

      virtual bool Check(const Binary::Container& rawData) const
      {
        return Format->Match(rawData);
      }

      virtual Formats::Multitrack::Container::Ptr Decode(const Binary::Container& rawData) const
      {
        if (!Format->Match(rawData))
        {
          return Formats::Multitrack::Container::Ptr();
        }
        const std::size_t availSize = rawData.Size();
        const RawHeader* const hdr = safe_ptr_cast<const RawHeader*>(rawData.Start());
        const std::size_t headersSize = sizeof(*hdr) + hdr->ExtraHeaderSize;
        const std::size_t bankSize = 0 != (hdr->ExtraBanks & 0x80) ? 8192 : 16384;
        const uint_t banksCount = hdr->ExtraBanks & 0x7f;
        const std::size_t totalSize = headersSize + fromLE(hdr->InitialDataSize) + bankSize * banksCount;
        //GME support truncated files
        const Binary::Container::Ptr used = rawData.GetSubcontainer(0, std::min(availSize, totalSize));
        const ExtraHeader* const extraHdr = safe_ptr_cast<const ExtraHeader*>(hdr + 1);
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
