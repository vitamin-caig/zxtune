/**
* 
* @file
*
* @brief  SID support implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "formats/chiptune/emulation/sid.h"
//common includes
#include <byteorder.h>
#include <contract.h>
#include <make_ptr.h>
#include <pointers.h>
//library includes
#include <binary/container_factories.h>
#include <binary/format_factories.h>
#include <math/numeric.h>
//std includes
#include <array>
#include <cstring>
//text includes
#include <formats/text/chiptune.h>

namespace Formats
{
namespace Chiptune
{
  namespace SID
  {
    typedef std::array<uint8_t, 4> SignatureType;

    const SignatureType SIGNATURE_RSID = {{'R', 'S', 'I', 'D'}};
    const SignatureType SIGNATURE_PSID = {{'P', 'S', 'I', 'D'}};

    const uint_t VERSION_MIN = 1;
    const uint_t VERSION_MAX = 3;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
    PACK_PRE struct RawHeader
    {
      SignatureType Signature;
      uint16_t Version;
      uint16_t DataOffset;
      uint16_t LoadAddr;
      uint16_t InitAddr;
      uint16_t PlayAddr;
      uint16_t SoungsCount;
      uint16_t StartSong;
      uint32_t SpeedFlags;
    } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

    static_assert(sizeof(RawHeader) == 22, "Invalid layout");

    const StringView FORMAT =
        "'R|'P 'S'I'D" //signature
        "00 01-03"     //BE version
        "00 76|7c"     //BE data offset
        "??"           //BE load address
        "??"           //BE init address
        "??"           //BE play address
        "00|01 ?"      //BE songs count 1-256
        "??"           //BE start song
        "????"         //BE speed flag
     ;

    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      Decoder()
        : Format(Binary::CreateMatchOnlyFormat(FORMAT))
      {
      }

      String GetDescription() const override
      {
        return Text::SID_DECODER_DESCRIPTION;
      }

      Binary::Format::Ptr GetFormat() const override
      {
        return Format;
      }

      bool Check(Binary::View rawData) const override
      {
        return Format->Match(rawData);
      }

      Formats::Chiptune::Container::Ptr Decode(const Binary::Container& /*rawData*/) const override
      {
        return Formats::Chiptune::Container::Ptr();//TODO
      }
    private:
      const Binary::Format::Ptr Format;
    };

    const RawHeader* GetHeader(Binary::View rawData)
    {
      if (rawData.Size() < sizeof(RawHeader))
      {
        return nullptr;
      }
      const RawHeader* hdr = safe_ptr_cast<const RawHeader*>(rawData.Start());
      if (hdr->Signature != SIGNATURE_PSID && hdr->Signature != SIGNATURE_RSID)
      {
        return nullptr;
      }
      if (!Math::InRange<uint_t>(fromBE(hdr->Version), VERSION_MIN, VERSION_MAX))
      {
        return nullptr;
      }
      return hdr;
    }

    uint_t GetModulesCount(Binary::View rawData)
    {
      if (const RawHeader* hdr = GetHeader(rawData))
      {
        return fromBE(hdr->SoungsCount);
      }
      else
      {
        return 0;
      }
    }

    Binary::Container::Ptr FixStartSong(Binary::View data, uint_t idx)
    {
      Require(GetHeader(data));
      std::unique_ptr<Dump> content(new Dump(data.Size()));
      std::memcpy(content->data(), data.Start(), content->size());
      RawHeader& hdr = *safe_ptr_cast<RawHeader*>(content->data());
      hdr.StartSong = fromBE<uint16_t>(idx);
      return Binary::CreateContainer(std::move(content));
    }
  }

  Decoder::Ptr CreateSIDDecoder()
  {
    return MakePtr<SID::Decoder>();
  }
}
}
