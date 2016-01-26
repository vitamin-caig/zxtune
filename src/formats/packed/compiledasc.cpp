/**
* 
* @file
*
* @brief  ASCSoundMaster compiled modules support
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "container.h"
#include "formats/chiptune/aym/ascsoundmaster.h"
//common includes
#include <byteorder.h>
#include <contract.h>
//library includes
#include <binary/format_factories.h>
#include <binary/typed_container.h>
#include <debug/log.h>
//boost includes
#include <boost/array.hpp>
#include <boost/ref.hpp>
#include <boost/make_shared.hpp>
//text includes
#include <formats/text/chiptune.h>
#include <formats/text/packed.h>

namespace
{
  const Debug::Stream Dbg("Formats::Packed::CompiledASC");
}

namespace CompiledASC
{
  const std::size_t MAX_MODULE_SIZE = 0x3a00;
  const std::size_t MAX_PLAYER_SIZE = 1700;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  typedef boost::array<uint8_t, 63> InfoData;
  
  PACK_PRE struct PlayerVer0
  {
    uint8_t Padding1[12];
    uint16_t InitAddr;
    uint8_t Padding2;
    uint16_t PlayAddr;
    uint8_t Padding3;
    uint16_t ShutAddr;
    //+20
    InfoData Information;
    //+83
    uint8_t Initialization;
    uint8_t Padding4[29];
    uint16_t DataAddr;

    std::size_t GetSize() const
    {
      const uint_t initAddr = fromLE(InitAddr);
      const uint_t compileAddr = initAddr - offsetof(PlayerVer0, Initialization);
      return fromLE(DataAddr) - compileAddr;
    }
  } PACK_POST;

  PACK_PRE struct PlayerVer2
  {
    uint8_t Padding1[20];
    //+20
    InfoData Information;
    //+83
    uint8_t Initialization;
    uint8_t Padding2[40];
    //+124
    uint16_t DataOffset;

    std::size_t GetSize() const
    {
      return DataOffset;
    }
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(offsetof(PlayerVer0, Information) == 20);
  BOOST_STATIC_ASSERT(offsetof(PlayerVer0, Initialization) == 83);
  BOOST_STATIC_ASSERT(offsetof(PlayerVer2, Initialization) == 83);
  BOOST_STATIC_ASSERT(offsetof(PlayerVer2, DataOffset) == 124);
  
  struct PlayerTraits
  {
    const std::size_t Size;
    const std::size_t InfoOffset;
    
    template<class Player>
    explicit PlayerTraits(const Player& player)
      : Size(player.GetSize())
      , InfoOffset(offsetof(Player, Information))
    {
    }

    template<class Player>
    static PlayerTraits Create(const Binary::TypedContainer& data)
    {
      const Player* const pl = data.GetField<Player>(0);
      Require(pl != 0);
      return PlayerTraits(*pl);
    }
  };
  
  typedef PlayerTraits (*CreatePlayerFunc)(const Binary::TypedContainer&);
  typedef Formats::Chiptune::Container::Ptr (*ParseFunc)(const Binary::Container&, Formats::Chiptune::ASCSoundMaster::Builder&);
  typedef Binary::Container::Ptr (*InsertMetaInfoFunc)(const Binary::Container&, const Dump&);
  
  struct VersionTraits
  {
    const std::size_t MinSize;
    const String Description;
    const std::string Format;
    const CreatePlayerFunc CreatePlayer;
    const ParseFunc Parse;
    const InsertMetaInfoFunc InsertMetaInformation;
  };

  const std::string ID_FORMAT =
    "'A'S'M' 'C'O'M'P'I'L'A'T'I'O'N' 'O'F' "
    "?{20}" //title
    "?{4}"  //any text
    "?{20}" //author
  ;

  const std::string BASE_FORMAT = 
    "?{11}" //unknown
    "c3??"  //init
    "c3??"  //play
    "c3??"  //silent
    + ID_FORMAT +
    //+0x53    init
    "af"       //xor a
    "?{28}"
  ;
  
  const VersionTraits VERSION0 =
  {
    sizeof(PlayerVer0),
    String(Text::ASCSOUNDMASTER0_DECODER_DESCRIPTION) + Text::PLAYER_SUFFIX,
    BASE_FORMAT +
      //+0x70
      "11??"     //ld de,ModuleAddr
      "42"       //ld b,d
      "4b"       //ld c,e
      "1a"       //ld a,(de)
      "13"       //inc de
      "32??"     //ld (xxx),a
      "cd??"     //call xxxx
    ,
    &PlayerTraits::Create<PlayerVer0>,
    &Formats::Chiptune::ASCSoundMaster::Ver0::Parse,
    &Formats::Chiptune::ASCSoundMaster::Ver0::InsertMetaInformation,
  };

  const VersionTraits VERSION1 =
  {
    sizeof(PlayerVer0),
    String(Text::ASCSOUNDMASTER1_DECODER_DESCRIPTION) + Text::PLAYER_SUFFIX,
    BASE_FORMAT +
      //+0x70
      "11??"     //ld de,ModuleAddr
      "42"       //ld b,d
      "4b"       //ld c,e
      "1a"       //ld a,(de)
      "13"       //inc de
      "32??"     //ld (xxx),a
      "1a"       //ld a,(de)
      "13"       //inc de
      "32??"     //ld (xxx),a
    ,
    &PlayerTraits::Create<PlayerVer0>,
    &Formats::Chiptune::ASCSoundMaster::Ver1::Parse,
    &Formats::Chiptune::ASCSoundMaster::Ver1::InsertMetaInformation,
  };

  const VersionTraits VERSION2 =
  {
    sizeof(PlayerVer2),
    String(Text::ASCSOUNDMASTER2_DECODER_DESCRIPTION) + Text::PLAYER_SUFFIX,
      "?{11}"     //padding
      "184600"
      "c3??"
      "c3??"
      + ID_FORMAT +
      //+0x53 init
      "cd??"
      "3b3b"
      "?{35}"
      //+123
      "11??" //data offset
    ,
    &PlayerTraits::Create<PlayerVer2>,
    &Formats::Chiptune::ASCSoundMaster::Ver1::Parse,
    &Formats::Chiptune::ASCSoundMaster::Ver1::InsertMetaInformation,
  };

  bool IsInfoEmpty(const InfoData& info)
  {
    //19 - fixed
    //20 - author
    //4  - ignore
    //20 - title
    const uint8_t* const authorStart = info.begin() + 19;
    const uint8_t* const ignoreStart = authorStart + 20;
    const uint8_t* const titleStart = ignoreStart + 4;
    return ignoreStart == std::find_if(authorStart, ignoreStart, std::bind2nd(std::greater<Char>(), Char(' ')))
        && info.end() == std::find_if(titleStart, info.end(), std::bind2nd(std::greater<Char>(), Char(' ')));
  }
}//CompiledASC

namespace Formats
{
  namespace Packed
  {
    class CompiledASCDecoder : public Decoder
    {
    public:
      explicit CompiledASCDecoder(const CompiledASC::VersionTraits& version)
        : Version(version)
        , Player(Binary::CreateFormat(Version.Format, Version.MinSize))
      {
      }

      virtual String GetDescription() const
      {
        return Version.Description;
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
        const CompiledASC::PlayerTraits rawPlayer = Version.CreatePlayer(typedData);
        if (rawPlayer.Size >= std::min(availSize, CompiledASC::MAX_PLAYER_SIZE))
        {
          Dbg("Invalid player");
          return Container::Ptr();
        }
        Dbg("Detected player in first %1% bytes", rawPlayer.Size);
        const std::size_t modDataSize = std::min(CompiledASC::MAX_MODULE_SIZE, availSize - rawPlayer.Size);
        const Binary::Container::Ptr modData = rawData.GetSubcontainer(rawPlayer.Size, modDataSize);
        const CompiledASC::InfoData& rawInfo = *typedData.GetField<CompiledASC::InfoData>(rawPlayer.InfoOffset);
        const Dump metainfo(rawInfo.begin(), rawInfo.end());
        if (CompiledASC::IsInfoEmpty(rawInfo))
        {
          Dbg("Player has empty metainfo");
          if (const Binary::Container::Ptr originalModule = Version.Parse(*modData, Formats::Chiptune::ASCSoundMaster::GetStubBuilder()))
          {
            const std::size_t originalSize = originalModule->Size();
            return CreatePackedContainer(originalModule, rawPlayer.Size + originalSize);
          }
        }
        else if (const Binary::Container::Ptr fixedModule = Version.InsertMetaInformation(*modData, metainfo))
        {
          if (Version.Parse(*fixedModule, Formats::Chiptune::ASCSoundMaster::GetStubBuilder()))
          {
            const std::size_t originalSize = fixedModule->Size() - metainfo.size();
            return CreatePackedContainer(fixedModule, rawPlayer.Size + originalSize);
          }
          Dbg("Failed to parse fixed module");
        }
        Dbg("Failed to find module after player");
        return Container::Ptr();
      }
    private:
      const CompiledASC::VersionTraits& Version;
      const Binary::Format::Ptr Player;
    };

    Decoder::Ptr CreateCompiledASC0Decoder()
    {
      return boost::make_shared<CompiledASCDecoder>(boost::cref(CompiledASC::VERSION0));
    }

    Decoder::Ptr CreateCompiledASC1Decoder()
    {
      return boost::make_shared<CompiledASCDecoder>(boost::cref(CompiledASC::VERSION1));
    }

    Decoder::Ptr CreateCompiledASC2Decoder()
    {
      return boost::make_shared<CompiledASCDecoder>(boost::cref(CompiledASC::VERSION2));
    }
  }//namespace Packed
}//namespace Formats
