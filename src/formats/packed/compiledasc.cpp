/*
Abstract:
  ASC Sound Master compiled modules support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//library includes
#include "container.h"
#include <formats/chiptune/ascsoundmaster.h>
//common includes
#include <byteorder.h>
#include <logging.h>
#include <tools.h>
//library includes
#include <binary/typed_container.h>
//boost includes
#include <boost/array.hpp>
//text includes
#include <formats/text/chiptune.h>
#include <formats/text/packed.h>

namespace
{
  const std::string THIS_MODULE("Formats::Packed::CompiledASC");
}

namespace CompiledASC
{
#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  typedef boost::array<uint8_t, 63> InfoData;

  struct Version0
  {
    static const String DESCRIPTION;
    static const std::string FORMAT;

    static Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Formats::Chiptune::ASCSoundMaster::Builder& target)
    {
      return Formats::Chiptune::ASCSoundMaster::ParseVersion0x(data, target);
    }

    PACK_PRE struct Player
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
        const uint_t compileAddr = initAddr - offsetof(Player, Initialization);
        return fromLE(DataAddr) - compileAddr;
      }
    } PACK_POST;

    PACK_PRE struct Header
    {
      uint8_t Tempo;
      uint16_t PatternsOffset;
      uint16_t SamplesOffset;
      uint16_t OrnamentsOffset;
      uint8_t Length;
      uint8_t Positions[1];
    } PACK_POST;
  };

  struct Version1 : Version0
  {
    static const String DESCRIPTION;
    static const std::string FORMAT;

    static Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Formats::Chiptune::ASCSoundMaster::Builder& target)
    {
      return Formats::Chiptune::ASCSoundMaster::ParseVersion1x(data, target);
    }

    PACK_PRE struct Header
    {
      uint8_t Tempo;
      uint8_t Loop;
      uint16_t PatternsOffset;
      uint16_t SamplesOffset;
      uint16_t OrnamentsOffset;
      uint8_t Length;
      uint8_t Positions[1];
    } PACK_POST;
  };

  struct Version2 : Version1
  {
    static const String DESCRIPTION;
    static const std::string FORMAT;

    PACK_PRE struct Player
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
  };
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(offsetof(Version1::Player, Information) == 20);
  BOOST_STATIC_ASSERT(offsetof(Version1::Player, Initialization) == 83);
  BOOST_STATIC_ASSERT(offsetof(Version2::Player, Initialization) == 83);
  BOOST_STATIC_ASSERT(offsetof(Version2::Player, DataOffset) == 124);

  const String Version0::DESCRIPTION = String(Text::ASCSOUNDMASTER0_DECODER_DESCRIPTION) + Text::PLAYER_SUFFIX;
  const String Version1::DESCRIPTION = String(Text::ASCSOUNDMASTER1_DECODER_DESCRIPTION) + Text::PLAYER_SUFFIX;
  const String Version2::DESCRIPTION = String(Text::ASCSOUNDMASTER2_DECODER_DESCRIPTION) + Text::PLAYER_SUFFIX;

  const std::string BASE_FORMAT(
    "+11+"  //unknown
    "c3??"  //init
    "c3??"  //play
    "c3??"  //silent
    "'A'S'M' 'C'O'M'P'I'L'A'T'I'O'N' 'O'F' "
    //+0x27
    "+44+"
    //+0x53    init
    "af"       //xor a
    "+28+"
  );

  const std::string Version0::FORMAT = BASE_FORMAT +
    //+0x70
    "11??"     //ld de,ModuleAddr
    "42"       //ld b,d
    "4b"       //ld c,e
    "1a"       //ld a,(de)
    "13"       //inc de
    "32??"     //ld (xxx),a
    "cd??"     //call xxxx
  ;  

  const std::string Version1::FORMAT = BASE_FORMAT +
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
  ;

  const std::string Version2::FORMAT(
    "+11+"     //padding
    "184600"
    "c3??"
    "c3??"
    "'A'S'M' 'C'O'M'P'I'L'A'T'I'O'N' 'O'F' "
    //+0x27
    "+44+"
    //+0x53 init
    "cd??"
    "3b3b"
    "+35+"
    //+123
    "11??" //data offset
  );

  template<class T, class D>
  T AddLE(T val, D delta)
  {
    return fromLE(static_cast<T>(fromLE(val) + delta));
  }

  template<class Version>
  Binary::Container::Ptr BuildFixedModule(const Formats::Chiptune::Container& module, const InfoData& info)
  {
    const std::size_t srcSize = module.Size();
    const std::size_t sizeAddon = sizeof(InfoData);
    std::auto_ptr<Dump> result(new Dump(srcSize + sizeAddon));
    const uint8_t* const srcData = safe_ptr_cast<const uint8_t*>(module.Data());
    const typename Version::Header& srcMod = *safe_ptr_cast<const typename Version::Header*>(srcData);
    const std::size_t hdrSize = offsetof(typename Version::Header, Positions) + srcMod.Length;
    const Dump::iterator endOfHeader = std::copy(srcData, srcData + hdrSize, result->begin());
    const Dump::iterator endOfInformation = std::copy(info.begin(), info.end(), endOfHeader);
    std::copy(srcData + hdrSize, srcData + srcSize, endOfInformation);
    typename Version::Header& dstMod = *safe_ptr_cast<typename Version::Header*>(&result->at(0));
    dstMod.PatternsOffset = AddLE(dstMod.PatternsOffset, sizeAddon);
    dstMod.SamplesOffset = AddLE(dstMod.SamplesOffset, sizeAddon);
    dstMod.OrnamentsOffset = AddLE(dstMod.OrnamentsOffset, sizeAddon);
    return Binary::CreateContainer(result);
  }

  template<class Version>
  class ModuleDecoder
  {
  public:
    explicit ModuleDecoder(const Binary::Container& data)
      : Data(data)
      , Delegate(Data)
      , Header(*Delegate.GetField<typename Version::Player>(0))
    {
    }

    bool FastCheck() const
    {
      return Header.GetSize() < Data.Size();
    }

    Formats::Packed::Container::Ptr GetResult() const
    {
      assert(FastCheck());
      const std::size_t modOffset = Header.GetSize();
      const Binary::Container::Ptr modData = Data.GetSubcontainer(modOffset, Data.Size() - modOffset);
      if (Formats::Chiptune::Container::Ptr module = Version::Parse(*modData, Formats::Chiptune::ASCSoundMaster::GetStubBuilder()))
      {
        const Binary::Container::Ptr result = BuildFixedModule<Version>(*module, Header.Information);
        return CreatePackedContainer(result, modOffset + module->Size());
      }
      Log::Debug(THIS_MODULE, "Failed to find module after player");
      return Formats::Packed::Container::Ptr();
    }
  private:
    const Binary::Container& Data;
    const Binary::TypedContainer Delegate;
    const typename Version::Player& Header;
  };
}//CompiledASC

namespace Formats
{
  namespace Packed
  {
    template<class Version>
    class CompiledASCDecoder : public Decoder
    {
    public:
      CompiledASCDecoder()
        : Player(Binary::Format::Create(Version::FORMAT))
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

      virtual bool Check(const Binary::Container& rawData) const
      {
        const void* const data = rawData.Data();
        const std::size_t availSize = rawData.Size();
        return Player->Match(data, availSize) && CompiledASC::ModuleDecoder<Version>(rawData).FastCheck();
      }

      virtual Container::Ptr Decode(const Binary::Container& rawData) const
      {
        const void* const data = rawData.Data();
        const std::size_t availSize = rawData.Size();
        if (!Player->Match(data, availSize))
        {
          return Container::Ptr();
        }
        const CompiledASC::ModuleDecoder<Version> decoder(rawData);
        if (!decoder.FastCheck())
        {
          return Container::Ptr();
        }
        return decoder.GetResult();
      }
    private:
      const Binary::Format::Ptr Player;
    };

    Decoder::Ptr CreateCompiledASC0Decoder()
    {
      return boost::make_shared<CompiledASCDecoder<CompiledASC::Version0> >();
    }

    Decoder::Ptr CreateCompiledASC1Decoder()
    {
      return boost::make_shared<CompiledASCDecoder<CompiledASC::Version1> >();
    }

    Decoder::Ptr CreateCompiledASC2Decoder()
    {
      return boost::make_shared<CompiledASCDecoder<CompiledASC::Version2> >();
    }
  }//namespace Packed
}//namespace Formats
