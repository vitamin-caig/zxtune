/**
* 
* @file
*
* @brief  VGM support implementation
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
//boost includes
#include <boost/array.hpp>
//text includes
#include <formats/text/chiptune.h>

namespace Formats
{
namespace Chiptune
{
  namespace VideoGameMusic
  {
    typedef boost::array<uint8_t, 4> SignatureType;

    const SignatureType SIGNATURE = {{'V', 'g', 'm', ' '}};

    const uint_t VERSION_MIN = 100;
    const uint_t VERSION_MAX = 170;
    
#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
    PACK_PRE struct VersionType
    {
      uint8_t Minor;
      uint8_t Major;
      uint16_t Unused;
      
      bool IsValid() const
      {
        return IsValid(Minor & 15) && IsValid(Minor >> 4) &&
               IsValid(Major & 15) && IsValid(Major >> 4) && Unused == 0;
      }
      
      uint_t GetValue() const
      {
        return (Minor & 15) + 10 * (Minor >> 4) + 100 * (Major & 15) + 1000 * (Major >> 4);
      }
      
    private:
      static bool IsValid(uint_t nibble)
      {
        return Math::InRange<uint_t>(nibble, 0, 9);
      }
    } PACK_POST;

    PACK_PRE struct MinimalHeader
    {
      SignatureType Signature;
      uint32_t EofOffset;
      VersionType Version;
      
      bool IsValid() const
      {
        return Signature == SIGNATURE
            && Version.IsValid() && Math::InRange(Version.GetValue(), VERSION_MIN, VERSION_MAX);
      }
      
      std::size_t Size()
      {
        const uint_t vers = Version.GetValue();
        if (vers <= 150)
        {
          return 0x40;
        }
        else if (vers <= 160)
        {
          return 0x80;
        }
        else if (vers <= 161)
        {
          return 0xc0;
        }
        else
        {
          return 0x100;
        }
      }
    } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

    BOOST_STATIC_ASSERT(sizeof(MinimalHeader) == 12);
    
    const std::size_t MIN_SIZE = 256;

    const std::string FORMAT =
        "'V'g'm' " //signature
        "????"     //eof offset
        //version
        "00-09|10-19|20-29|30-39|40-49|50-59|60-69|70"
        "01 00 00"
     ;

    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      Decoder()
        : Format(Binary::CreateFormat(FORMAT, MIN_SIZE))
      {
      }

      virtual String GetDescription() const
      {
        return Text::VGM_DECODER_DESCRIPTION;
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
        const MinimalHeader& hdr = *safe_ptr_cast<const MinimalHeader*>(rawData.Start());
        const std::size_t totalSize = offsetof(MinimalHeader, EofOffset) + fromLE(hdr.EofOffset);
        if (hdr.IsValid() && totalSize <= rawData.Size())
        {
          const Binary::Container::Ptr data = rawData.GetSubcontainer(0, totalSize);
          return CreateCalculatingCrcContainer(data, 0, totalSize);
        }
          return Formats::Chiptune::Container::Ptr();
      }
    private:
      const Binary::Format::Ptr Format;
    };
  }

  Decoder::Ptr CreateVideoGameMusicDecoder()
  {
    return MakePtr<VideoGameMusic::Decoder>();
  }
}
}
