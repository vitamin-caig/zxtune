/**
 *
 * @file
 *
 * @brief  SoundTrackerPro compiled modules support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/aym/soundtrackerpro.h"
#include "formats/packed/container.h"

#include "binary/format_factories.h"
#include "debug/log.h"

#include "byteorder.h"
#include "make_ptr.h"
#include "string_view.h"

#include <algorithm>
#include <array>
#include <cstring>

namespace Formats::Packed
{
  namespace CompiledSTP
  {
    const Debug::Stream Dbg("Formats::Packed::CompiledSTP");

    const std::size_t MAX_MODULE_SIZE = 0x2800;
    const std::size_t MAX_PLAYER_SIZE = 2000;

    using RawInformation = std::array<uint8_t, 53>;

    struct Version1
    {
      static const StringView DESCRIPTION;
      static const StringView FORMAT;

      struct RawPlayer
      {
        uint8_t Padding1;
        le_uint16_t DataAddr;
        uint8_t Padding2;
        le_uint16_t InitAddr;
        uint8_t Padding3;
        le_uint16_t PlayAddr;
        uint8_t Padding4[8];
        //+17
        RawInformation Information;
        uint8_t Padding5[8];
        //+78
        uint8_t Initialization;

        std::size_t GetSize() const
        {
          const uint_t initAddr = InitAddr;
          const uint_t compileAddr = initAddr - offsetof(RawPlayer, Initialization);
          return DataAddr - compileAddr;
        }

        RawInformation GetInfo() const
        {
          return Information;
        }
      };
    };

    struct Version2
    {
      static const StringView DESCRIPTION;
      static const StringView FORMAT;

      struct RawPlayer
      {
        uint8_t Padding1;
        le_uint16_t InitAddr;
        uint8_t Padding2;
        le_uint16_t PlayAddr;
        uint8_t Padding3[2];
        //+8
        uint8_t Information[56];
        uint8_t Padding4[8];
        //+0x48
        uint8_t Initialization[2];
        le_uint16_t DataAddr;

        std::size_t GetSize() const
        {
          const uint_t initAddr = InitAddr;
          const uint_t compileAddr = initAddr - offsetof(RawPlayer, Initialization);
          return DataAddr - compileAddr;
        }

        RawInformation GetInfo() const
        {
          RawInformation result;
          const uint8_t* const src = Information;
          uint8_t* const dst = result.data();
          std::memcpy(dst, src, 24);
          std::memcpy(dst + 24, src + 26, 4);
          std::memcpy(dst + 27, src + 31, 25);
          return result;
        }
      };
    };

    static_assert(offsetof(Version1::RawPlayer, Information) == 17, "Invalid layout");
    static_assert(offsetof(Version1::RawPlayer, Initialization) == 78, "Invalid layout");
    static_assert(offsetof(Version2::RawPlayer, Information) == 8, "Invalid layout");
    static_assert(offsetof(Version2::RawPlayer, Initialization) == 72, "Invalid layout");

    const StringView Version1::DESCRIPTION = "Sound Tracker Pro v1.x player"sv;
    const StringView Version2::DESCRIPTION = "Sound Tracker Pro v2.x player"sv;

    const StringView Version1::FORMAT =
        "21??"    // ld hl,ModuleAddr
        "c3??"    // jp xxxx
        "c3??"    // jp xxxx
        "ed4b??"  // ld bc,(xxxx)
        "c3??"    // jp xxxx
        "?"       // nop?
        "'K'S'A' 'S'O'F'T'W'A'R'E' 'C'O'M'P'I'L'A'T'I'O'N' 'O'F' "
        "?{25}"
        "?{8}"
        "f3"    // di
        "22??"  // ld (xxxx),hl
        "3e?"   // ld a,xx
        "32??"  // ld (xxxx),a
        "32??"  // ld (xxxx),a
        "32??"  // ld (xxxx),a
        "7e"    // ld a,(hl)
        "23"    // inc hl
        "32??"  // ld (xxxx),a
        ""sv;

    const StringView Version2::FORMAT =
        "c3??"  // jp InitAddr
        "c3??"  // jp PlayAddr
        "??"    // nop,nop
        "'K'S'A' 'S'O'F'T'W'A'R'E' 'C'O'M'P'I'L'A'T'I'O'N' ' ' 'O'F' ' ' "
        "?{24}"
        "?{8}"
        //+0x48
        "f3"    // di
        "21??"  // ld hl,ModuleAddr
        "22??"  // ld (xxxx),hl
        "3e?"   // ld a,xx
        "32??"  // ld (xxxx),a
        "32??"  // ld (xxxx),a
        "32??"  // ld (xxxx),a
        "7e"    // ld a,(hl)
        "23"    // inc hl
        "32??"  // ld (xxxx),a
        ""sv;

    bool IsInfoEmpty(const RawInformation& info)
    {
      // 28 is fixed
      // 25 is title
      const auto* const titleStart = info.begin() + 28;
      return std::none_of(titleStart, info.end(), [](auto b) { return b > ' '; });
    }
  }  // namespace CompiledSTP

  template<class Version>
  class CompiledSTPDecoder : public Decoder
  {
  public:
    CompiledSTPDecoder()
      : Player(Binary::CreateFormat(Version::FORMAT, sizeof(typename Version::RawPlayer)))
    {}

    StringView GetDescription() const override
    {
      return Version::DESCRIPTION;
    }

    Binary::Format::Ptr GetFormat() const override
    {
      return Player;
    }

    Container::Ptr Decode(const Binary::Container& rawData) const override
    {
      using namespace CompiledSTP;
      const Binary::View data(rawData);
      if (!Player->Match(data))
      {
        return {};
      }
      const auto& rawPlayer = *data.As<typename Version::RawPlayer>();
      const auto playerSize = rawPlayer.GetSize();
      if (playerSize >= std::min(data.Size(), CompiledSTP::MAX_PLAYER_SIZE))
      {
        Dbg("Invalid player");
        return {};
      }
      Dbg("Detected player in first {} bytes", playerSize);
      const auto modData = rawData.GetSubcontainer(playerSize, CompiledSTP::MAX_MODULE_SIZE);
      const auto metainfo = rawPlayer.GetInfo();
      auto& stub = Formats::Chiptune::SoundTrackerPro::GetStubBuilder();
      if (CompiledSTP::IsInfoEmpty(metainfo))
      {
        Dbg("Player has empty metainfo");
        if (auto originalModule = Formats::Chiptune::SoundTrackerPro::ParseCompiled(*modData, stub))
        {
          const std::size_t originalSize = originalModule->Size();
          return CreateContainer(std::move(originalModule), playerSize + originalSize);
        }
      }
      else if (auto fixedModule = Formats::Chiptune::SoundTrackerPro::InsertMetaInformation(*modData, metainfo))
      {
        if (Formats::Chiptune::SoundTrackerPro::ParseCompiled(*fixedModule, stub))
        {
          const std::size_t originalSize = fixedModule->Size() - metainfo.size();
          return CreateContainer(std::move(fixedModule), playerSize + originalSize);
        }
        Dbg("Failed to parse fixed module");
      }
      Dbg("Failed to find module after player");
      return {};
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
}  // namespace Formats::Packed
