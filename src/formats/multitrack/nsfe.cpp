/**
* 
* @file
*
* @brief  NSFE support implementation
*
* @author vitamin.caig@gmail.com
*
**/

//common includes
#include <byteorder.h>
#include <contract.h>
#include <crc.h>
#include <pointers.h>
//library includes
#include <binary/container_factories.h>
#include <binary/format_factories.h>
#include <binary/input_stream.h>
#include <formats/multitrack.h>
#include <math/numeric.h>
//std includes
#include <cstring>
//boost includes
#include <boost/array.hpp>
#include <boost/make_shared.hpp>

namespace Formats
{
namespace Multitrack
{
  namespace NSFE
  {
    typedef boost::array<uint8_t, 4> ChunkIdType;
    
    const ChunkIdType NSFE = { {'N', 'S', 'F', 'E'} };
    const ChunkIdType INFO = { {'I', 'N', 'F', 'O'} };
    const ChunkIdType DATA = { {'D', 'A', 'T', 'A'} };
    const ChunkIdType NEND = { {'N', 'E', 'N', 'D'} };
    
    typedef boost::array<char, 32> StringType;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
    PACK_PRE struct ChunkHeader
    {
      uint32_t Size;
      ChunkIdType Id;
    } PACK_POST;
    
    PACK_PRE struct InfoChunk
    {
      uint16_t LoadAddr;
      uint16_t InitAddr;
      uint16_t PlayAddr;
      uint8_t Mode;
      uint8_t ExtraDevices;
    } PACK_POST;
    
    PACK_PRE struct InfoChunkFull : InfoChunk
    {
      uint8_t TracksCount;
      uint8_t StartTrack; //0-based
    } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

    BOOST_STATIC_ASSERT(sizeof(ChunkHeader) == 8);
    BOOST_STATIC_ASSERT(sizeof(InfoChunk) == 8);
    BOOST_STATIC_ASSERT(sizeof(InfoChunkFull) == 10);
    
    const std::size_t MAX_SIZE = 1048576;

    const std::string FORMAT =
      "'N'S'F'E"
      "08-ff 00 00 00" //sizeof INFO
      "'I'N'F'O"
      //gme supports nfs load/init address starting from 0x8000 or zero
      "(? 80-ff){2}"
     ;
     
    const std::size_t MIN_SIZE = 256;

    class Container : public Formats::Multitrack::Container
    {
    public:
      Container(const InfoChunkFull* info, uint32_t fixedCrc, Binary::Container::Ptr data)
        : Info(info)
        , FixedCrc(fixedCrc)
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
        return FixedCrc;
      }

      virtual uint_t TracksCount() const
      {
        return Info ? Info->TracksCount : 1;
      }

      virtual uint_t StartTrackIndex() const
      {
        return Info ? Info->StartTrack - 1 : 0;
      }
      
      virtual Container::Ptr WithStartTrackIndex(uint_t idx) const
      {
        Require(Info != 0);
        const std::size_t infoOffset = safe_ptr_cast<const uint8_t*>(Info) - static_cast<const uint8_t*>(Delegate->Start());
        std::auto_ptr<Dump> content(new Dump(Delegate->Size()));
        std::memcpy(&content->front(), Delegate->Start(), content->size());
        InfoChunkFull* const info = safe_ptr_cast<InfoChunkFull*>(&content->front() + infoOffset);
        Require(idx < info->TracksCount);
        info->StartTrack = idx;
        return boost::make_shared<Container>(info, FixedCrc, Binary::CreateContainer(content));
      }
    private:
      const InfoChunkFull* const Info;
      const uint32_t FixedCrc;
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
        try
        {
          Binary::InputStream input(rawData);
          Require(input.ReadField<ChunkIdType>() == NSFE);
          const InfoChunkFull* info = 0;
          uint32_t fixedCrc = 0;
          for (;;)
          {
            const ChunkHeader& hdr = input.ReadField<ChunkHeader>();
            const std::size_t size = fromLE(hdr.Size);
            if (hdr.Id == NEND)
            {
              Require(size == 0);
              break;
            }
            const uint8_t* const data = input.ReadData(size);
            if (hdr.Id == INFO)
            {
              fixedCrc = Crc32(data, sizeof(InfoChunk), fixedCrc);
              if (size >= sizeof(InfoChunkFull))
              {
                info = safe_ptr_cast<const InfoChunkFull*>(data);
              }
            }
            else if (hdr.Id == DATA)
            {
              fixedCrc = Crc32(data, size, fixedCrc);
            }
          }
          return boost::make_shared<Container>(info, fixedCrc, input.GetReadData());
        }
        catch (const std::exception&)
        {
        }
        return Formats::Multitrack::Container::Ptr();
      }
    private:
      const Binary::Format::Ptr Format;
    };
  }//namespace NSFE

  Decoder::Ptr CreateNSFEDecoder()
  {
    return boost::make_shared<NSFE::Decoder>();
  }
}//namespace Multitrack
}//namespace Formats
