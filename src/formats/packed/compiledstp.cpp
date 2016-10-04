/**
* 
* @file
*
* @brief  SoundTrackerPro compiled modules support
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "container.h"
#include "formats/chiptune/aym/soundtrackerpro.h"
//common includes
#include <byteorder.h>
#include <make_ptr.h>
//library includes
#include <binary/format_factories.h>
#include <binary/typed_container.h>
#include <debug/log.h>
//std includes
#include <array>
#include <cstring>
//text includes
#include <formats/text/chiptune.h>
#include <formats/text/packed.h>

namespace Formats
{
namespace Packed
{
  namespace CompiledSTP
  {
    const Debug::Stream Dbg("Formats::Packed::CompiledSTP");

    const std::size_t MAX_MODULE_SIZE = 0x2800;
    const std::size_t MAX_PLAYER_SIZE = 2000;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
    struct Version1
    {
      static const String DESCRIPTION;
      static const std::string FORMAT;

      PACK_PRE struct RawPlayer
      {
        uint8_t Padding1;
        uint16_t DataAddr;
        uint8_t Padding2;
        uint16_t InitAddr;
        uint8_t Padding3;
        uint16_t PlayAddr;
        uint8_t Padding4[8];
        //+17
        std::array<uint8_t, 53> Information;
        uint8_t Padding5[8];
        //+78
        uint8_t Initialization;

        std::size_t GetSize() const
        {
          const uint_t initAddr = fromLE(InitAddr);
          const uint_t compileAddr = initAddr - offsetof(RawPlayer, Initialization);
          return fromLE(DataAddr) - compileAddr;
        }

        Dump GetInfo() const
        {
          return Dump(Information.begin(), Information.end());
        }
      } PACK_POST;
    };

    struct Version2
    {
      static const String DESCRIPTION;
      static const std::string FORMAT;

      PACK_PRE struct RawPlayer
      {
        uint8_t Padding1;
        uint16_t InitAddr;
        uint8_t Padding2;
        uint16_t PlayAddr;
        uint8_t Padding3[2];
        //+8
        uint8_t Information[56];
        uint8_t Padding4[8];
        //+0x48
        uint8_t Initialization[2];
        uint16_t DataAddr;

        std::size_t GetSize() const
        {
          const uint_t initAddr = fromLE(InitAddr);
          const uint_t compileAddr = initAddr - offsetof(RawPlayer, Initialization);
          return fromLE(DataAddr) - compileAddr;
        }

        Dump GetInfo() const
        {
          Dump result(53);
          const uint8_t* const src = Information;
          uint8_t* const dst = &result[0];
          std::memcpy(dst, src, 24);
          std::memcpy(dst + 24, src + 26, 4);
          std::memcpy(dst + 27, src + 31, 25);
          return result;
        }
      } PACK_POST;
    };
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

    static_assert(offsetof(Version1::RawPlayer, Information) == 17, "Invalid layout");
    static_assert(offsetof(Version1::RawPlayer, Initialization) == 78, "Invalid layout");
    static_assert(offsetof(Version2::RawPlayer, Information) == 8, "Invalid layout");
    static_assert(offsetof(Version2::RawPlayer, Initialization) == 72, "Invalid layout");

    const String Version1::DESCRIPTION = String(Text::SOUNDTRACKERPRO_DECODER_DESCRIPTION) + Text::PLAYER_SUFFIX;
    const String Version2::DESCRIPTION = String(Text::SOUNDTRACKERPRO2_DECODER_DESCRIPTION) + Text::PLAYER_SUFFIX;

    const std::string Version1::FORMAT =
      "21??"     //ld hl,ModuleAddr
      "c3??"     //jp xxxx
      "c3??"     //jp xxxx
      "ed4b??"   //ld bc,(xxxx)
      "c3??"     //jp xxxx
      "?"        //nop?
      "'K'S'A' 'S'O'F'T'W'A'R'E' 'C'O'M'P'I'L'A'T'I'O'N' 'O'F' "
      "?{25}"
      "?{8}"
      "f3"       //di
      "22??"     //ld (xxxx),hl
      "3e?"      //ld a,xx
      "32??"     //ld (xxxx),a
      "32??"     //ld (xxxx),a
      "32??"     //ld (xxxx),a
      "7e"       //ld a,(hl)
      "23"       //inc hl
      "32??"     //ld (xxxx),a
    ;

    const std::string Version2::FORMAT =
      "c3??"     //jp InitAddr
      "c3??"     //jp PlayAddr
      "??"       //nop,nop
      "'K'S'A' 'S'O'F'T'W'A'R'E' 'C'O'M'P'I'L'A'T'I'O'N' ' ' 'O'F' ' ' "
      "?{24}"
      "?{8}"
      //+0x48
      "f3"       //di
      "21??"     //ld hl,ModuleAddr
      "22??"     //ld (xxxx),hl
      "3e?"      //ld a,xx
      "32??"     //ld (xxxx),a
      "32??"     //ld (xxxx),a
      "32??"     //ld (xxxx),a
      "7e"       //ld a,(hl)
      "23"       //inc hl
      "32??"     //ld (xxxx),a
    ;

    bool IsInfoEmpty(const Dump& info)
    {
      assert(info.size() == 53);
      //28 is fixed
      //25 is title
      const Dump::const_iterator titleStart = info.begin() + 28;
      return info.end() == std::find_if(titleStart, info.end(), std::bind2nd(std::greater<Char>(), Char(' ')));
    }
  }//CompiledSTP

  template<class Version>
  class CompiledSTPDecoder : public Decoder
  {
  public:
    CompiledSTPDecoder()
      : Player(Binary::CreateFormat(Version::FORMAT, sizeof(typename Version::RawPlayer)))
    {
    }

    virtual String GetDescription() const
    {
      return Version::DESCRIPTION;
    }

    virtual Binary::Format::Ptr GetFormat() const
    {
      return Player;
    }

    virtual Container::Ptr Decode(const Binary::Container& rawData) const
    {
      using namespace CompiledSTP;
      if (!Player->Match(rawData))
      {
        return Container::Ptr();
      }
      const Binary::TypedContainer typedData(rawData);
      const std::size_t availSize = rawData.Size();
      const typename Version::RawPlayer& rawPlayer = *typedData.GetField<typename Version::RawPlayer>(0);
      const std::size_t playerSize = rawPlayer.GetSize();
      if (playerSize >= std::min(availSize, CompiledSTP::MAX_PLAYER_SIZE))
      {
        Dbg("Invalid player");
        return Container::Ptr();
      }
      Dbg("Detected player in first %1% bytes", playerSize);
      const std::size_t modDataSize = std::min(CompiledSTP::MAX_MODULE_SIZE, availSize - playerSize);
      const Binary::Container::Ptr modData = rawData.GetSubcontainer(playerSize, modDataSize);
      const Dump metainfo = rawPlayer.GetInfo();
      if (CompiledSTP::IsInfoEmpty(metainfo))
      {
        Dbg("Player has empty metainfo");
        if (const Binary::Container::Ptr originalModule = Formats::Chiptune::SoundTrackerPro::ParseCompiled(*modData, Formats::Chiptune::SoundTrackerPro::GetStubBuilder()))
        {
          const std::size_t originalSize = originalModule->Size();
          return CreateContainer(originalModule, playerSize + originalSize);
        }
      }
      else if (const Binary::Container::Ptr fixedModule = Formats::Chiptune::SoundTrackerPro::InsertMetaInformation(*modData, metainfo))
      {
        if (Formats::Chiptune::SoundTrackerPro::ParseCompiled(*fixedModule, Formats::Chiptune::SoundTrackerPro::GetStubBuilder()))
        {
          const std::size_t originalSize = fixedModule->Size() - metainfo.size();
          return CreateContainer(fixedModule, playerSize + originalSize);
        }
        Dbg("Failed to parse fixed module");
      }
      Dbg("Failed to find module after player");
      return Container::Ptr();
    }
  private:
    const Binary::Format::Ptr Player;
  };

  Decoder::Ptr CreateCompiledSTP1Decoder()
  {
    return MakePtr<CompiledSTPDecoder<CompiledSTP::Version1> >();
  }

  Decoder::Ptr CreateCompiledSTP2Decoder()
  {
    return MakePtr<CompiledSTPDecoder<CompiledSTP::Version2> >();
  }
}//namespace Packed
}//namespace Formats
