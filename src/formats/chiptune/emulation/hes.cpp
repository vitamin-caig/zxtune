/**
* 
* @file
*
* @brief  HES support implementation
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
#include <binary/format_factories.h>
#include <formats/chiptune/container.h>
#include <math/numeric.h>
//std includes
#include <cstring>
//boost includes
#include <boost/array.hpp>
//text includes
#include <formats/text/chiptune.h>

namespace Formats
{
namespace Chiptune
{
  namespace HES
  {
    typedef boost::array<uint8_t, 4> SignatureType;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
    PACK_PRE struct RawHeader
    {
      SignatureType Signature;
      uint8_t Version;
      uint8_t StartSong;
      uint16_t RequestAddress;
      uint8_t InitialMPR[8];
      SignatureType DataSignature;
      uint32_t DataSize;
      uint32_t DataAddress;
      uint32_t Unused;
      //uint8_t Unused2[0x20];
      //uint8_t Fields[0x90];
    } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

    BOOST_STATIC_ASSERT(sizeof(RawHeader) == 0x20);

    const std::string FORMAT =
        "'H'E'S'M" //signature
        "?"        //version
        "?"        //start song
        "??"       //requested address
        "?{8}"     //MPR
        "'D'A'T'A" //data signature
        "? ? 0x 00"//1MB size limit
        "? ? 0x 00"//1MB size limit
     ;

    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      Decoder()
        : Format(Binary::CreateFormat(FORMAT))
      {
      }

      virtual String GetDescription() const
      {
        return Text::HES_DECODER_DESCRIPTION;
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
        if (!Format->Match(rawData))
        {
          return Formats::Chiptune::Container::Ptr();
        }
        const RawHeader& hdr = *safe_ptr_cast<const RawHeader*>(rawData.Start());
        const std::size_t totalSize = sizeof(hdr) + fromLE(hdr.DataSize);
        //GME support truncated files
        const std::size_t realSize = std::min(rawData.Size(), totalSize);
        const Binary::Container::Ptr data = rawData.GetSubcontainer(0, realSize);
        return CreateCalculatingCrcContainer(data, 0, realSize);
      }
    private:
      const Binary::Format::Ptr Format;
    };
  }

  Decoder::Ptr CreateHESDecoder()
  {
    return MakePtr<HES::Decoder>();
  }
}
}
