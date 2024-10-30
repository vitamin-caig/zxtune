/**
 *
 * @file
 *
 * @brief  SoundTracker v3.x compiled modules support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "formats/chiptune/aym/soundtracker.h"
#include "formats/packed/container.h"
// common includes
#include <byteorder.h>
#include <make_ptr.h>
// library includes
#include <binary/format_factories.h>
#include <debug/log.h>
// std includes
#include <algorithm>
#include <array>

namespace Formats::Packed
{
  namespace CompiledST3
  {
    const Debug::Stream Dbg("Formats::Packed::CompiledST3");

    const std::size_t MAX_MODULE_SIZE = 0x4000;
    const std::size_t MAX_PLAYER_SIZE = 0xa00;

    using RawInformation = std::array<uint8_t, 55>;

    struct RawPlayer
    {
      uint8_t Padding1;
      le_uint16_t DataAddr;
      uint8_t Padding2;
      le_uint16_t InitAddr;
      uint8_t Padding3;
      le_uint16_t PlayAddr;
      uint8_t Padding4[3];
      //+12
      RawInformation Information;
      //+67
      uint8_t Initialization;

      uint_t GetCompileAddr() const
      {
        const uint_t initAddr = InitAddr;
        return initAddr - offsetof(RawPlayer, Initialization);
      }

      std::size_t GetSize() const
      {
        const uint_t compileAddr = GetCompileAddr();
        return DataAddr - compileAddr;
      }

      const RawInformation& GetInfo() const
      {
        return Information;
      }
    };

    static_assert(offsetof(RawPlayer, Information) == 12, "Invalid layout");
    static_assert(offsetof(RawPlayer, Initialization) == 67, "Invalid layout");

    const auto DESCRIPTION = "Sound Tracker v3.x Compiled player"sv;

    const auto FORMAT =
        "21??"  // ld hl,ModuleAddr
        "c3??"  // jp xxxx
        "c3??"  // jp xxxx
        "c3??"  // jp xxx
        "'K'S'A' 'S'O'F'T'W'A'R'E' 'C'O'M'P'I'L'A'T'I'O'N' 'O'F' "
        "?{27}"
        //+0x43
        "f3"    // di
        "7e"    // ld a,(hl)
        "32??"  // ld (xxxx),a
        "22??"  // ld (xxxx),hl
        "22??"  // ld (xxxx),hl
        "23"    // inc hl
        ""sv;

    bool IsInfoEmpty(const RawInformation& info)
    {
      // 28 is fixed
      // 27 is title
      const auto* const titleStart = info.begin() + 28;
      return std::none_of(titleStart, info.end(), [](auto c) { return c > ' '; });
    }
  }  // namespace CompiledST3

  class CompiledST3Decoder : public Decoder
  {
  public:
    CompiledST3Decoder()
      : Player(Binary::CreateFormat(CompiledST3::FORMAT, sizeof(CompiledST3::RawPlayer)))
    {}

    StringView GetDescription() const override
    {
      return CompiledST3::DESCRIPTION;
    }

    Binary::Format::Ptr GetFormat() const override
    {
      return Player;
    }

    Container::Ptr Decode(const Binary::Container& rawData) const override
    {
      using namespace CompiledST3;

      const Binary::View data(rawData);
      if (!Player->Match(data))
      {
        return {};
      }
      const auto& rawPlayer = *data.As<RawPlayer>();
      const std::size_t playerSize = rawPlayer.GetSize();
      if (playerSize >= std::min(data.Size(), MAX_PLAYER_SIZE))
      {
        Dbg("Invalid compile addr");
        return {};
      }
      const uint_t compileAddr = rawPlayer.GetCompileAddr();
      Dbg("Detected player compiled at #{:04x} in first {} bytes", compileAddr, playerSize);
      const auto modData = rawData.GetSubcontainer(playerSize, MAX_MODULE_SIZE);
      const auto metainfo = rawPlayer.GetInfo();
      auto& stub = Formats::Chiptune::SoundTracker::GetStubBuilder();
      if (IsInfoEmpty(metainfo))
      {
        Dbg("Player has empty metainfo");
        if (auto originalModule = Formats::Chiptune::SoundTracker::Ver3::Parse(*modData, stub))
        {
          const std::size_t originalSize = originalModule->Size();
          return CreateContainer(std::move(originalModule), playerSize + originalSize);
        }
      }
      else if (auto fixedModule = Formats::Chiptune::SoundTracker::Ver3::InsertMetainformation(*modData, metainfo))
      {
        if (Formats::Chiptune::SoundTracker::Ver3::Parse(*fixedModule, stub))
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

  Decoder::Ptr CreateCompiledST3Decoder()
  {
    return MakePtr<CompiledST3Decoder>();
  }
}  // namespace Formats::Packed
