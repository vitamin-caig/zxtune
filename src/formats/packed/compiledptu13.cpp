/**
 *
 * @file
 *
 * @brief  ProTrackerUtility v1.3 compiled modules support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/aym/protracker3_detail.h"
#include "formats/chiptune/metainfo.h"
#include "formats/packed/container.h"

#include <binary/format_factories.h>
#include <debug/log.h>

#include <byteorder.h>
#include <make_ptr.h>

#include <array>

namespace Formats::Packed
{
  namespace CompiledPTU13
  {
    const Debug::Stream Dbg("Formats::Packed::CompiledPTU13");

    const std::size_t MAX_MODULE_SIZE = 0xb900;
    const std::size_t PLAYER_SIZE = 0x900;

    namespace ProTracker3 = Formats::Chiptune::ProTracker3;

    struct RawPlayer
    {
      uint8_t Padding1[0x1b];
      le_uint16_t PositionsAddr;
    };

    struct RawHeader
    {
      uint8_t Id[13];  //'ProTracker 3.'
      uint8_t Subversion;
      uint8_t Optional1[16];  //' compilation of '
      uint8_t Metainfo[68];
      uint8_t Mode;
      uint8_t FreqTableNum;
      uint8_t Tempo;
      uint8_t Length;
      uint8_t Loop;
      le_uint16_t PatternsOffset;
      std::array<le_uint16_t, ProTracker3::MAX_SAMPLES_COUNT> SamplesOffsets;
      std::array<le_uint16_t, ProTracker3::MAX_ORNAMENTS_COUNT> OrnamentsOffsets;
      uint8_t Positions[1];  // finished by marker
    };

    const uint8_t POS_END_MARKER = 0xff;

    static_assert(sizeof(RawHeader) * alignof(RawHeader) == 202, "Invalid layout");

    const auto DESCRIPTION = "Pro Tracker Utility v1.3 player"sv;

    const auto FORMAT =
        "21??"    // ld hl,xxxx +0x665
        "35"      // dec (hl)
        "c2??"    // jp nz,xxxx
        "23"      // inc hl
        "35"      // dec (hl)
        "2045"    // jr nz,xx
        "11??"    // ld de,xxxx +0x710
        "1a"      // ld a,(de)
        "b7"      // or a
        "2029"    // jr nz,xx
        "32??"    // ld (xxxx),a
        "57"      // ld d,a
        "ed73??"  // ld (xxxx),sp
        "21??"    // ld hl,xxxx //positions offset
        "7e"      // ld a,(hl)
        "3c"      // inc a
        "2003"    // jr nz,xxxx
        "21??"    // ld hl,xxxx
        "5e"      // ld e,(hl)
        "23"      // inc hl
        "22??"    // ld (xxxx),hl
        "21??"    // ld hl,xxxx
        "19"      // add hl,de
        "19"      // add hl,de
        "f9"      // ld sp,hl
        "d1"      // pop de
        "e1"      // pop hl
        "22??"    // ld (xxxx),hl
        ""sv;

    uint_t GetPatternsCount(const RawHeader& hdr, std::size_t maxSize)
    {
      const uint8_t* const dataBegin = &hdr.Tempo;
      const uint8_t* const dataEnd = dataBegin + maxSize;
      const uint8_t* const lastPosition = std::find(hdr.Positions, dataEnd, POS_END_MARKER);
      if (lastPosition != dataEnd && std::all_of(hdr.Positions, lastPosition, [](auto p) { return 0 == p % 3; }))
      {
        return 1 + *std::max_element(hdr.Positions, lastPosition) / 3;
      }
      return 0;
    }
  }  // namespace CompiledPTU13

  class CompiledPTU13Decoder : public Decoder
  {
  public:
    CompiledPTU13Decoder()
      : Player(
          Binary::CreateFormat(CompiledPTU13::FORMAT, CompiledPTU13::PLAYER_SIZE + sizeof(CompiledPTU13::RawHeader)))
    {}

    StringView GetDescription() const override
    {
      return CompiledPTU13::DESCRIPTION;
    }

    Binary::Format::Ptr GetFormat() const override
    {
      return Player;
    }

    Container::Ptr Decode(const Binary::Container& rawData) const override
    {
      namespace ProTracker3 = Formats::Chiptune::ProTracker3;
      using namespace CompiledPTU13;

      const Binary::View data(rawData);
      if (!Player->Match(data))
      {
        return {};
      }
      const std::size_t playerSize = CompiledPTU13::PLAYER_SIZE;
      const auto& rawPlayer = *data.As<CompiledPTU13::RawPlayer>();
      const uint_t positionsAddr = rawPlayer.PositionsAddr;
      if (positionsAddr < playerSize + offsetof(CompiledPTU13::RawHeader, Positions))
      {
        Dbg("Invalid compile addr");
        return {};
      }
      const uint_t dataAddr = positionsAddr - offsetof(CompiledPTU13::RawHeader, Positions);
      const auto modData = data.SubView(playerSize, CompiledPTU13::MAX_MODULE_SIZE);
      const auto& rawHeader = *modData.As<CompiledPTU13::RawHeader>();
      const uint_t patternsCount = CompiledPTU13::GetPatternsCount(rawHeader, modData.Size());
      if (!patternsCount)
      {
        Dbg("Invalid patterns count");
        return {};
      }
      const uint_t compileAddr = dataAddr - playerSize;
      Dbg("Detected player compiled at #%{:04x}) with {} patterns", compileAddr, patternsCount);
      const auto builder = Formats::Chiptune::PatchedDataBuilder::Create(modData);
      // fix patterns/samples/ornaments offsets
      for (uint_t idx = offsetof(CompiledPTU13::RawHeader, PatternsOffset);
           idx != offsetof(CompiledPTU13::RawHeader, Positions); idx += 2)
      {
        builder->FixLEWord(idx, -int_t(dataAddr));
      }
      // fix patterns offsets
      for (uint_t idx = rawHeader.PatternsOffset - dataAddr, lim = idx + 6 * patternsCount; idx != lim; idx += 2)
      {
        builder->FixLEWord(idx, -int_t(dataAddr));
      }
      const auto fixedModule = builder->GetResult();
      if (auto fixedParsed = ProTracker3::Parse(*fixedModule, ProTracker3::GetStubBuilder()))
      {
        const auto totalSize = playerSize + fixedParsed->Size();
        return CreateContainer(std::move(fixedParsed), totalSize);
      }
      Dbg("Failed to parse fixed module");
      return {};
    }

  private:
    const Binary::Format::Ptr Player;
  };

  Decoder::Ptr CreateCompiledPTU13Decoder()
  {
    return MakePtr<CompiledPTU13Decoder>();
  }
}  // namespace Formats::Packed
