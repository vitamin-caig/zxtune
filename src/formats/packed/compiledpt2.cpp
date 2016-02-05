/**
* 
* @file
*
* @brief  ProTracker v2.40 Phantom Family compiled modules support
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "container.h"
#include "formats/chiptune/metainfo.h"
#include "formats/chiptune/aym/protracker2.h"
//common includes
#include <byteorder.h>
#include <make_ptr.h>
//library includes
#include <binary/format_factories.h>
#include <binary/typed_container.h>
#include <debug/log.h>
//boost includes
#include <boost/array.hpp>
//text includes
#include <formats/text/chiptune.h>
#include <formats/text/packed.h>

namespace
{
  const Debug::Stream Dbg("Formats::Packed::CompiledPT24");
}

namespace Formats
{
namespace Packed
{
  namespace CompiledPT24
  {
    const std::size_t MAX_MODULE_SIZE = 0x3600;
    const std::size_t PLAYER_SIZE = 0xa45;
    const std::size_t MAX_PATTERNS_COUNT = 32;
    const std::size_t MAX_SAMPLES_COUNT = 32;
    const std::size_t MAX_ORNAMENTS_COUNT = 16;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
    PACK_PRE struct Player
    {
      uint8_t Padding1;
      uint16_t DataAddr;
    } PACK_POST;

    PACK_PRE struct RawHeader
    {
      uint8_t Tempo;
      uint8_t Length;
      uint8_t Loop;
      boost::array<uint16_t, MAX_SAMPLES_COUNT> SamplesOffsets;
      boost::array<uint16_t, MAX_ORNAMENTS_COUNT> OrnamentsOffsets;
      uint16_t PatternsOffset;
      char Name[30];
      uint8_t Positions[1];
    } PACK_POST;

    const uint8_t POS_END_MARKER = 0xff;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

    const String DESCRIPTION = String(Text::PROTRACKER24_DECODER_DESCRIPTION) + Text::PLAYER_SUFFIX;

    const std::string FORMAT(
      "21??"  //ld hl,xxxx
      "1803"  //jr xx
      "c3??"  //jp xxxx
      "f3"    //di
      "e5"    //push hl
      "7e"    //ld a,(hl)
      "32??"  //ld (xxxx),a
      "32??"  //ld (xxxx),a
      "23"    //inc hl
      "23"    //inc hl
      "7e"    //ld a,(hl)
      "23"    //inc hl
      "11??"  //ld de,xxxx
      "22??"  //ld (xxxx),hl
      "22??"  //ld (xxxx),hl
      "22??"  //ld (xxxx),hl
      "19"    //add hl,de
      "19"    //add hl,de
    );

    uint_t GetPatternsCount(const RawHeader& hdr, std::size_t maxSize)
    {
      const uint8_t* const dataBegin = &hdr.Tempo;
      const uint8_t* const dataEnd = dataBegin + maxSize;
      const uint8_t* const lastPosition = std::find(hdr.Positions, dataEnd, POS_END_MARKER);
      if (lastPosition != dataEnd && 
          lastPosition == std::find_if(hdr.Positions, lastPosition, std::bind2nd(std::greater_equal<std::size_t>(), MAX_PATTERNS_COUNT)))
      {
        return 1 + *std::max_element(hdr.Positions, lastPosition);
      }
      return 0;
    }
  }//CompiledPT24

  class CompiledPT24Decoder : public Decoder
  {
  public:
    CompiledPT24Decoder()
      : Player(Binary::CreateFormat(CompiledPT24::FORMAT, CompiledPT24::PLAYER_SIZE + sizeof(CompiledPT24::RawHeader)))
      , Decoder(Formats::Chiptune::CreateProTracker2Decoder())
    {
    }

    virtual String GetDescription() const
    {
      return CompiledPT24::DESCRIPTION;
    }

    virtual Binary::Format::Ptr GetFormat() const
    {
      return Player;
    }

    virtual Container::Ptr Decode(const Binary::Container& rawData) const
    {
      if (!Player->Match(rawData))
      {
        return Container::Ptr();
      }
      const Binary::TypedContainer typedData(rawData);
      const std::size_t availSize = rawData.Size();
      const std::size_t playerSize = CompiledPT24::PLAYER_SIZE;
      const CompiledPT24::Player& rawPlayer = *typedData.GetField<CompiledPT24::Player>(0);
      const uint_t dataAddr = fromLE(rawPlayer.DataAddr);
      if (dataAddr < playerSize)
      {
        Dbg("Invalid compile addr");
        return Container::Ptr();
      }
      const CompiledPT24::RawHeader& rawHeader = *typedData.GetField<CompiledPT24::RawHeader>(playerSize);
      const uint_t patternsCount = CompiledPT24::GetPatternsCount(rawHeader, availSize - playerSize);
      if (!patternsCount)
      {
        Dbg("Invalid patterns count");
        return Container::Ptr();
      }
      const uint_t compileAddr = dataAddr - playerSize;
      Dbg("Detected player compiled at %1% (#%1$04x) with %2% patterns", compileAddr, patternsCount);
      const std::size_t modDataSize = std::min(CompiledPT24::MAX_MODULE_SIZE, availSize - playerSize);
      const Binary::Container::Ptr modData = rawData.GetSubcontainer(playerSize, modDataSize);
      const Formats::Chiptune::PatchedDataBuilder::Ptr builder = Formats::Chiptune::PatchedDataBuilder::Create(*modData);
      //fix samples/ornaments offsets
      for (uint_t idx = offsetof(CompiledPT24::RawHeader, SamplesOffsets); idx != offsetof(CompiledPT24::RawHeader, PatternsOffset); idx += 2)
      {
        builder->FixLEWord(idx, -int_t(dataAddr));
      }
      //fix patterns offsets
      for (uint_t idx = fromLE(rawHeader.PatternsOffset), lim = idx + 6 * patternsCount; idx != lim; idx += 2)
      {
        builder->FixLEWord(idx, -int_t(dataAddr));
      }
      const Binary::Container::Ptr fixedModule = builder->GetResult();
      if (Formats::Chiptune::Container::Ptr fixedParsed = Decoder->Decode(*fixedModule))
      {
        return CreateContainer(fixedParsed, playerSize + fixedParsed->Size());
      }
      Dbg("Failed to parse fixed module");
      return Container::Ptr();
    }
  private:
    const Binary::Format::Ptr Player;
    const Formats::Chiptune::Decoder::Ptr Decoder;
  };

  Decoder::Ptr CreateCompiledPT24Decoder()
  {
    return MakePtr<CompiledPT24Decoder>();
  }
}//namespace Packed
}//namespace Formats
