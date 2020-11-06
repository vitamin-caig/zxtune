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
#include <make_ptr.h>
#include <pointers.h>
//library includes
#include <binary/container_base.h>
#include <binary/container_factories.h>
#include <binary/crc.h>
#include <binary/format_factories.h>
#include <binary/input_stream.h>
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
  namespace NSFE
  {
    typedef std::array<uint8_t, 4> ChunkIdType;
    
    const ChunkIdType NSFE = { {'N', 'S', 'F', 'E'} };
    const ChunkIdType INFO = { {'I', 'N', 'F', 'O'} };
    const ChunkIdType DATA = { {'D', 'A', 'T', 'A'} };
    const ChunkIdType NEND = { {'N', 'E', 'N', 'D'} };
    
    typedef std::array<char, 32> StringType;

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

    static_assert(sizeof(ChunkHeader) == 8, "Invalid layout");
    static_assert(sizeof(InfoChunk) == 8, "Invalid layout");
    static_assert(sizeof(InfoChunkFull) == 10, "Invalid layout");
    
    //const std::size_t MAX_SIZE = 1048576;

    const StringView FORMAT =
      "'N'S'F'E"
      "08-ff 00 00 00" //sizeof INFO
      "'I'N'F'O"
      //gme supports nfs load/init address starting from 0x8000 or zero
      "(? 80-ff){2}"
     ;
     
    const std::size_t MIN_SIZE = 256;

    class Container : public Binary::BaseContainer<Multitrack::Container>
    {
    public:
      Container(const InfoChunkFull* info, uint32_t fixedCrc, Binary::Container::Ptr data)
        : BaseContainer(std::move(data))
        , Info(info)
        , FixedCrc(fixedCrc)
      {
      }
      
      uint_t FixedChecksum() const override
      {
        return FixedCrc;
      }

      uint_t TracksCount() const override
      {
        return Info ? Info->TracksCount : 1;
      }

      uint_t StartTrackIndex() const override
      {
        return Info ? Info->StartTrack - 1 : 0;
      }
      
      Container::Ptr WithStartTrackIndex(uint_t idx) const override
      {
        Require(Info != nullptr);
        const Binary::View data(*Delegate);
        const std::size_t infoOffset = safe_ptr_cast<const uint8_t*>(Info) - data.As<uint8_t>();
        std::unique_ptr<Dump> content(new Dump(data.Size()));
        std::memcpy(content->data(), data.Start(), data.Size());
        InfoChunkFull* const info = safe_ptr_cast<InfoChunkFull*>(content->data() + infoOffset);
        Require(idx < info->TracksCount);
        info->StartTrack = idx;
        return MakePtr<Container>(info, FixedCrc, Binary::CreateContainer(std::move(content)));
      }
    private:
      const InfoChunkFull* const Info;
      const uint32_t FixedCrc;
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
        try
        {
          Binary::InputStream input(rawData);
          Require(input.ReadField<ChunkIdType>() == NSFE);
          const InfoChunkFull* info = nullptr;
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
            const auto data = input.ReadData(size);
            if (hdr.Id == INFO)
            {
              fixedCrc = Binary::Crc32(data.SubView(0, sizeof(InfoChunk)), fixedCrc);
              if (size >= sizeof(InfoChunkFull))
              {
                info = safe_ptr_cast<const InfoChunkFull*>(data.Start());
              }
            }
            else if (hdr.Id == DATA)
            {
              fixedCrc = Binary::Crc32(data, fixedCrc);
            }
          }
          return MakePtr<Container>(info, fixedCrc, input.GetReadContainer());
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
    return MakePtr<NSFE::Decoder>();
  }
}//namespace Multitrack
}//namespace Formats
