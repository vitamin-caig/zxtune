/*
Abstract:
  Sound Tracker v3.x compiled modules support (player)

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "container.h"
#include "formats/chiptune/aym/soundtracker.h"
//common includes
#include <byteorder.h>
#include <tools.h>
//library includes
#include <binary/typed_container.h>
#include <debug/log.h>
//boost includes
#include <boost/array.hpp>
//text includes
#include <formats/text/chiptune.h>
#include <formats/text/packed.h>

namespace
{
  const Debug::Stream Dbg("Formats::Packed::CompiledST3");
}

namespace CompiledST3
{
  const std::size_t MAX_MODULE_SIZE = 0x4000;
  const std::size_t MAX_PLAYER_SIZE = 0xa00;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct Player
  {
    uint8_t Padding1;
    uint16_t DataAddr;
    uint8_t Padding2;
    uint16_t InitAddr;
    uint8_t Padding3;
    uint16_t PlayAddr;
    uint8_t Padding4[3];
    //+12
    uint8_t Information[55];
    //+67
    uint8_t Initialization;

    uint_t GetCompileAddr() const
    {
      const uint_t initAddr = fromLE(InitAddr);
      return initAddr - offsetof(Player, Initialization);
    }

    std::size_t GetSize() const
    {
      const uint_t compileAddr = GetCompileAddr();
      return fromLE(DataAddr) - compileAddr;
    }

    Dump GetInfo() const
    {
      return Dump(Information, Information + 55);
    }
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif
  BOOST_STATIC_ASSERT(offsetof(Player, Information) == 12);
  BOOST_STATIC_ASSERT(offsetof(Player, Initialization) == 67); 

  const String DESCRIPTION = String(Text::SOUNDTRACKER3_DECODER_DESCRIPTION) + Text::PLAYER_SUFFIX;

  const std::string FORMAT(
    "21??"     //ld hl,ModuleAddr
    "c3??"     //jp xxxx
    "c3??"     //jp xxxx
    "c3??"     //jp xxx
    "'K'S'A' 'S'O'F'T'W'A'R'E' 'C'O'M'P'I'L'A'T'I'O'N' 'O'F' "
    "?{27}"
    //+0x43
    "f3"       //di
    "7e"       //ld a,(hl)
    "32??"     //ld (xxxx),a
    "22??"     //ld (xxxx),hl
    "22??"     //ld (xxxx),hl
    "23"       //inc hl 
  );

  bool IsInfoEmpty(const Dump& info)
  {
    assert(info.size() == 55);
    //28 is fixed
    //27 is title
    const Dump::const_iterator titleStart = info.begin() + 28;
    return info.end() == std::find_if(titleStart, info.end(), std::bind2nd(std::greater<Char>(), Char(' ')));
  }
}//CompiledST3

namespace Formats
{
  namespace Packed
  {
    class CompiledST3Decoder : public Decoder
    {
    public:
      CompiledST3Decoder()
        : Player(Binary::Format::Create(CompiledST3::FORMAT, sizeof(CompiledST3::Player)))
      {
      }

      virtual String GetDescription() const
      {
        return CompiledST3::DESCRIPTION;
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
        const CompiledST3::Player& rawPlayer = *typedData.GetField<CompiledST3::Player>(0);
        const std::size_t playerSize = rawPlayer.GetSize();
        if (playerSize >= std::min(availSize, CompiledST3::MAX_PLAYER_SIZE))
        {
          Dbg("Invalid compile addr");
          return Container::Ptr();
        }
        const uint_t compileAddr = rawPlayer.GetCompileAddr();
        Dbg("Detected player compiled at %1% (#%1$04x) in first %2% bytes", compileAddr, playerSize);
        const std::size_t modDataSize = std::min(availSize - playerSize, CompiledST3::MAX_MODULE_SIZE);
        const Binary::Container::Ptr modData = rawData.GetSubcontainer(playerSize, modDataSize);
        const Dump& metainfo = rawPlayer.GetInfo();
        Formats::Chiptune::SoundTracker::Builder& stub = Formats::Chiptune::SoundTracker::GetStubBuilder();
        if (CompiledST3::IsInfoEmpty(metainfo))
        {
          Dbg("Player has empty metainfo");
          if (const Binary::Container::Ptr originalModule = Formats::Chiptune::SoundTracker::Ver3::Parse(*modData, stub))
          {
            const std::size_t originalSize = originalModule->Size();
            return CreatePackedContainer(originalModule, playerSize + originalSize);
          }
        }
        else if (const Binary::Container::Ptr fixedModule = Formats::Chiptune::SoundTracker::Ver3::InsertMetainformation(*modData, metainfo))
        {
          if (Formats::Chiptune::SoundTracker::Ver3::Parse(*fixedModule, stub))
          {
            const std::size_t originalSize = fixedModule->Size() - metainfo.size();
            return CreatePackedContainer(fixedModule, playerSize + originalSize);
          }
          Dbg("Failed to parse fixed module");
        }
        Dbg("Failed to find module after player");
        return Container::Ptr();
      }
    private:
      const Binary::Format::Ptr Player;
    };

    Decoder::Ptr CreateCompiledST3Decoder()
    {
      return boost::make_shared<CompiledST3Decoder>();
    }
  }//namespace Packed
}//namespace Formats
