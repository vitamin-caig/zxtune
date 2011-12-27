/*
Abstract:
  Sound Tracker Pro compiled modules support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//library includes
#include "container.h"
#include <formats/chiptune/soundtrackerpro.h>
//common includes
#include <byteorder.h>
#include <logging.h>
#include <tools.h>
//library includes
#include <binary/typed_container.h>
//std includes
#include <set>
//boost includes
#include <boost/array.hpp>
//text includes
#include <formats/text/chiptune.h>
#include <formats/text/packed.h>

namespace
{
  const std::string THIS_MODULE("Formats::Packed::CompiledSTP");
}

namespace CompiledSTP
{
  const std::size_t MAX_PLAYER_SIZE = 2000;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  typedef boost::array<uint8_t, 53> InfoData;

  struct Version1
  {
    static const String DESCRIPTION;
    static const std::string FORMAT;

    PACK_PRE struct Player
    {
      uint8_t Padding1;
      uint16_t DataAddr;
      uint8_t Padding2;
      uint16_t InitAddr;
      uint8_t Padding3;
      uint16_t PlayAddr;
      uint8_t Padding4[8];
      //+17
      InfoData Information;
      uint8_t Padding5[8];
      //+78
      uint8_t Initialization;

      std::size_t GetSize() const
      {
        const uint_t initAddr = fromLE(InitAddr);
        const uint_t compileAddr = initAddr - offsetof(Player, Initialization);
        return fromLE(DataAddr) - compileAddr;
      }
    } PACK_POST;
  };

  PACK_PRE struct RawHeader
  {
    uint8_t Tempo;
    uint16_t PositionsOffset;
    uint16_t PatternsOffset;
    uint16_t OrnamentsOffset;
    uint16_t SamplesOffset;
    uint8_t FixesCount;
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(offsetof(Version1::Player, Information) == 17);
  BOOST_STATIC_ASSERT(offsetof(Version1::Player, Initialization) == 78);

  const String Version1::DESCRIPTION = String(Text::SOUNDTRACKERPRO_DECODER_DESCRIPTION) + Text::PLAYER_SUFFIX;

  const std::string ID_FORMAT(
    "'K'S'A' 'S'O'F'T'W'A'R'E' 'C'O'M'P'I'L'A'T'I'O'N' 'O'F' "
    "+25+" //title
  );

  const std::string Version1::FORMAT =
    "21??"     //ld hl,ModuleAddr
    "c3??"     //jp xxxx
    "c3??"     //jp xxxx
    "ed4b??"   //ld bc,(xxxx)
    "c3??"     //jp xxxx
    "?"        //nop?
    + ID_FORMAT +
    "+8+"
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

  template<class T, class D>
  T AddLE(T val, D delta)
  {
    return fromLE(static_cast<T>(fromLE(val) + delta));
  }

  typedef std::set<std::size_t> OffsetsSet;

  Binary::Container::Ptr BuildFixedModule(const Formats::Chiptune::Container& module, const InfoData& info, const OffsetsSet& offsets)
  {
    const std::size_t srcSize = module.Size();
    const std::size_t sizeAddon = sizeof(InfoData);
    std::auto_ptr<Dump> result(new Dump(srcSize + sizeAddon));
    const uint8_t* const srcData = safe_ptr_cast<const uint8_t*>(module.Data());
    const std::size_t hdrSize = sizeof(RawHeader);
    const Dump::iterator endOfHeader = std::copy(srcData, srcData + hdrSize, result->begin());
    const Dump::iterator endOfInformation = std::copy(info.begin(), info.end(), endOfHeader);
    std::copy(srcData + hdrSize, srcData + srcSize, endOfInformation);
    for (OffsetsSet::const_iterator it = offsets.begin(), lim = offsets.end(); it != lim; ++it)
    {
      const std::size_t addr = *it;
      uint16_t& var = *safe_ptr_cast<uint16_t*>(&result->at(addr >= hdrSize ? addr + sizeAddon : addr));
      var = AddLE(var, sizeAddon);
    }
    return Binary::CreateContainer(result);
  }

  class FixesCollector : public Formats::Chiptune::SoundTrackerPro::Builder
  {
  public:
    explicit FixesCollector(const Binary::Container& data)
      : Header(*safe_ptr_cast<const RawHeader*>(data.Data()))
    {
      Result.insert(offsetof(RawHeader, PositionsOffset));
      Result.insert(offsetof(RawHeader, PatternsOffset));
      Result.insert(offsetof(RawHeader, OrnamentsOffset));
      Result.insert(offsetof(RawHeader, SamplesOffset));
      Result.insert(fromLE(Header.PatternsOffset));
    }

    virtual void SetProgram(const String& /*program*/) {}
    virtual void SetTitle(const String& /*title*/) {}
    virtual void SetInitialTempo(uint_t /*tempo*/) {}

    virtual void SetSample(uint_t index, const Formats::Chiptune::SoundTrackerPro::Sample& /*sample*/)
    {
      const std::size_t ptrOffset = fromLE(Header.SamplesOffset) + index * sizeof(uint16_t);
      Result.insert(ptrOffset);
    }

    virtual void SetOrnament(uint_t index, const Formats::Chiptune::SoundTrackerPro::Ornament& /*ornament*/)
    {
      const std::size_t ptrOffset = fromLE(Header.OrnamentsOffset) + index * sizeof(uint16_t);
      Result.insert(ptrOffset);
    }

    virtual void SetPositions(const std::vector<Formats::Chiptune::SoundTrackerPro::PositionEntry>& /*positions*/, uint_t /*loop*/) {}

    virtual void StartPattern(uint_t index)
    {
      const std::size_t ptrOffset = fromLE(Header.PatternsOffset) + index * 3 * sizeof(uint16_t);
      Result.insert(ptrOffset);
      Result.insert(ptrOffset + 2);
      Result.insert(ptrOffset + 4);
    }

    virtual void FinishPattern(uint_t /*size*/) {}
    virtual void StartLine(uint_t /*index*/) {}
    virtual void StartChannel(uint_t /*index*/) {}
    virtual void SetRest() {}
    virtual void SetNote(uint_t /*note*/) {}
    virtual void SetSample(uint_t /*sample*/) {}
    virtual void SetOrnament(uint_t /*ornament*/) {}
    virtual void SetEnvelope(uint_t /*type*/, uint_t /*value*/) {}
    virtual void SetNoEnvelope() {}
    virtual void SetGliss(uint_t /*target*/) {}
    virtual void SetVolume(uint_t /*vol*/) {}

    OffsetsSet GetResult()
    {
      return Result;
    }
  private:
    const RawHeader& Header;
    OffsetsSet Result;
  };

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
      return Header.GetSize() < std::min(Data.Size(), MAX_PLAYER_SIZE);
    }

    Formats::Packed::Container::Ptr GetResult() const
    {
      assert(FastCheck());
      const std::size_t modOffset = Header.GetSize();
      Log::Debug(THIS_MODULE, "Detected player in first %1% bytes", modOffset);
      const Binary::Container::Ptr modData = Data.GetSubcontainer(modOffset, Data.Size() - modOffset);
      FixesCollector fixes(*modData);
      if (Formats::Chiptune::Container::Ptr module = Formats::Chiptune::SoundTrackerPro::ParseCompiled(*modData, fixes))
      {
        const Binary::Container::Ptr result = BuildFixedModule(*module, Header.Information, fixes.GetResult());
        if (Formats::Chiptune::SoundTrackerPro::ParseCompiled(*result, Formats::Chiptune::SoundTrackerPro::GetStubBuilder()))
        {
          return CreatePackedContainer(result, modOffset + module->Size());
        }
        Log::Debug(THIS_MODULE, "Failed to parse fixed module");
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
    class CompiledSTPDecoder : public Decoder
    {
    public:
      CompiledSTPDecoder()
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
        return Player->Match(data, availSize) && CompiledSTP::ModuleDecoder<Version>(rawData).FastCheck();
      }

      virtual Container::Ptr Decode(const Binary::Container& rawData) const
      {
        const void* const data = rawData.Data();
        const std::size_t availSize = rawData.Size();
        if (!Player->Match(data, availSize))
        {
          return Container::Ptr();
        }
        const CompiledSTP::ModuleDecoder<Version> decoder(rawData);
        if (!decoder.FastCheck())
        {
          return Container::Ptr();
        }
        return decoder.GetResult();
      }
    private:
      const Binary::Format::Ptr Player;
    };

    Decoder::Ptr CreateCompiledSTP1Decoder()
    {
      return boost::make_shared<CompiledSTPDecoder<CompiledSTP::Version1> >();
    }
  }//namespace Packed
}//namespace Formats
