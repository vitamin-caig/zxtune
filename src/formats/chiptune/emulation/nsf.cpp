/**
* 
* @file
*
* @brief  NSF support implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "nsf.h"
//common includes
#include <byteorder.h>
#include <contract.h>
#include <crc.h>
#include <pointers.h>
//library includes
#include <binary/container_factories.h>
#include <binary/format_factories.h>
#include <math/numeric.h>
//std includes
#include <cstring>
//boost includes
#include <boost/array.hpp>
#include <boost/make_shared.hpp>
//text includes
#include <formats/text/chiptune.h>

namespace Formats
{
namespace Chiptune
{
  namespace NSF
  {
    typedef boost::array<uint8_t, 5> SignatureType;
    
    typedef boost::array<char, 32> StringType;

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

    BOOST_STATIC_ASSERT(sizeof(RawHeader) == 128);
    
    const std::size_t MAX_SIZE = 1048576;

    const std::string FORMAT =
      "'N'E'S'M"
      "1a"
      "?"     //version
      "01-ff" //1 song minimum
      "01-ff"
      //gme supports nfs load/init address starting from 0x8000 or zero
      "(? 80-ff){2}"
     ;
     
    const std::size_t MIN_SIZE = 256;

    const RawHeader* GetHeader(const Binary::Data& rawData)
    {
      if (rawData.Size() < MIN_SIZE)
      {
        return 0;
      }
      const RawHeader* hdr = safe_ptr_cast<const RawHeader*>(rawData.Start());
      if (hdr->Signature != SIGNATURE)
      {
        return 0;
      }
      if (!hdr->SongsCount || !hdr->StartSong)
      {
        return 0;
      }
      if (fromLE(hdr->LoadAddr) < 0x8000 || fromLE(hdr->InitAddr) < 0x8000)
      {
        return 0;
      }
      return hdr;
    }
    
    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      Decoder()
        : Format(Binary::CreateMatchOnlyFormat(FORMAT, MIN_SIZE))
      {
      }

      virtual String GetDescription() const
      {
        return Text::NSF_DECODER_DESCRIPTION;
      }

      virtual Binary::Format::Ptr GetFormat() const
      {
        return Format;
      }

      virtual bool Check(const Binary::Container& rawData) const
      {
        return Format->Match(rawData);
      }

      virtual Formats::Chiptune::Container::Ptr Decode(const Binary::Container& rawData) const
      {
        return Parse(rawData);
      }
    private:
      const Binary::Format::Ptr Format;
    };

    class Container : public PolyTrackContainer
    {
    public:
      Container(const RawHeader* hdr, Binary::Container::Ptr data)
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
      
      //Formats::Chiptune::Container
      virtual uint_t Checksum() const
      {
        return Crc32(static_cast<const uint8_t*>(Delegate->Start()), Delegate->Size());
      }

      virtual uint_t FixedChecksum() const
      {
        //just skip text fields
        const uint8_t* const data = static_cast<const uint8_t*>(Delegate->Start());
        const uint32_t part1 = Crc32(data, offsetof(RawHeader, Title));
        const uint32_t part2 = Crc32(data + offsetof(RawHeader, NTSCSpeedUs), Delegate->Size() - offsetof(RawHeader, NTSCSpeedUs), part1);
        return part2;
      }

      //Formats::Chiptune::PolyTrackContainer
      virtual uint_t TracksCount() const
      {
        return Hdr->SongsCount;
      }

      virtual uint_t StartTrackIndex() const
      {
        return Hdr->StartSong - 1;
      }
      
      virtual Binary::Container::Ptr WithStartTrackIndex(uint_t idx) const
      {
        std::auto_ptr<Dump> content(new Dump(Delegate->Size()));
        std::memcpy(&content->front(), Delegate->Start(), content->size());
        RawHeader& hdr = *safe_ptr_cast<RawHeader*>(&content->front());
        Require(idx < hdr.SongsCount);
        hdr.StartSong = idx + 1;
        return Binary::CreateContainer(content);
      }
    private:
      const RawHeader* const Hdr;
      const Binary::Container::Ptr Delegate;
    };

    PolyTrackContainer::Ptr Parse(const Binary::Container& data)
    {
      if (const RawHeader* hdr = GetHeader(data))
      {
        const Binary::Container::Ptr used = data.GetSubcontainer(0, std::min(data.Size(), MAX_SIZE));
        return boost::make_shared<Container>(hdr, used);
      }
      else
      {
        return PolyTrackContainer::Ptr();
      }
    }
  }

  Decoder::Ptr CreateNSFDecoder()
  {
    return boost::make_shared<NSF::Decoder>();
  }
}
}