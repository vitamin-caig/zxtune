/**
* 
* @file
*
* @brief  GYM support implementation
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
  namespace GYM
  {
    typedef boost::array<uint8_t, 4> SignatureType;
    typedef boost::array<uint8_t, 32> StringType;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
    PACK_PRE struct RawHeader
    {
      SignatureType Signature;
      StringType Song;
      StringType Game;
      StringType Copyright;
      StringType Emulator;
      StringType Dumper;
      boost::array<uint8_t, 256> Comment;
      uint32_t LoopStart;
      uint32_t PackedSize;
    } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

    static_assert(sizeof(RawHeader) == 428, "Invalid layout");
    
    const std::size_t MIN_SIZE = sizeof(RawHeader) + 256;
    const std::size_t MAX_SIZE = 16 * 1024 * 1024;

    const std::string FORMAT =
        "'G'Y'M'X" //signature
     ;

    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      Decoder()
        : Format(Binary::CreateMatchOnlyFormat(FORMAT, MIN_SIZE))
      {
      }

      virtual String GetDescription() const
      {
        return Text::GYM_DECODER_DESCRIPTION;
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
        const std::size_t realSize = std::min(rawData.Size(), MAX_SIZE);
        const Binary::Container::Ptr data = rawData.GetSubcontainer(0, realSize);
        return CreateCalculatingCrcContainer(data, 0, realSize);
      }
    private:
      const Binary::Format::Ptr Format;
    };
  }

  Decoder::Ptr CreateGYMDecoder()
  {
    return MakePtr<GYM::Decoder>();
  }
}
}
