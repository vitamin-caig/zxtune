/**
 *
 * @file
 *
 * @brief  ProTracker v2.40 Phantom Family compiled modules support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "formats/chiptune/aym/protracker2.h"
#include "formats/chiptune/metainfo.h"
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
  namespace CompiledPT24
  {
    const Debug::Stream Dbg("Formats::Packed::CompiledPT24");

    const std::size_t MAX_MODULE_SIZE = 0x3600;
    const std::size_t PLAYER_SIZE = 0xa45;
    const std::size_t MAX_PATTERNS_COUNT = 32;
    const std::size_t MAX_SAMPLES_COUNT = 32;
    const std::size_t MAX_ORNAMENTS_COUNT = 16;

    struct RawPlayer
    {
      uint8_t Padding1;
      le_uint16_t DataAddr;
    };

    struct RawHeader
    {
      uint8_t Tempo;
      uint8_t Length;
      uint8_t Loop;
      std::array<le_uint16_t, MAX_SAMPLES_COUNT> SamplesOffsets;
      std::array<le_uint16_t, MAX_ORNAMENTS_COUNT> OrnamentsOffsets;
      le_uint16_t PatternsOffset;
      char Name[30];
      uint8_t Positions[1];
    };

    static_assert(sizeof(RawHeader) * alignof(RawHeader) == 132, "Wrong layout");

    const uint8_t POS_END_MARKER = 0xff;

    const Char DESCRIPTION[] = "Pro Tracker v2.40 Phantom Family player";

    const auto FORMAT =
        "21??"  // ld hl,xxxx
        "1803"  // jr xx
        "c3??"  // jp xxxx
        "f3"    // di
        "e5"    // push hl
        "7e"    // ld a,(hl)
        "32??"  // ld (xxxx),a
        "32??"  // ld (xxxx),a
        "23"    // inc hl
        "23"    // inc hl
        "7e"    // ld a,(hl)
        "23"    // inc hl
        "11??"  // ld de,xxxx
        "22??"  // ld (xxxx),hl
        "22??"  // ld (xxxx),hl
        "22??"  // ld (xxxx),hl
        "19"    // add hl,de
        "19"    // add hl,de
        ""sv;

    uint_t GetPatternsCount(const RawHeader& hdr, std::size_t maxSize)
    {
      const uint8_t* const dataBegin = &hdr.Tempo;
      const uint8_t* const dataEnd = dataBegin + maxSize;
      const uint8_t* const lastPosition = std::find(hdr.Positions, dataEnd, POS_END_MARKER);
      if (lastPosition != dataEnd
          && std::none_of(hdr.Positions, lastPosition, [](auto b) { return b >= MAX_PATTERNS_COUNT; }))
      {
        return 1 + *std::max_element(hdr.Positions, lastPosition);
      }
      return 0;
    }
  }  // namespace CompiledPT24

  class CompiledPT24Decoder : public Decoder
  {
  public:
    CompiledPT24Decoder()
      : Player(Binary::CreateFormat(CompiledPT24::FORMAT, CompiledPT24::PLAYER_SIZE + sizeof(CompiledPT24::RawHeader)))
      , Decoder(Formats::Chiptune::CreateProTracker2Decoder())
    {}

    String GetDescription() const override
    {
      return CompiledPT24::DESCRIPTION;
    }

    Binary::Format::Ptr GetFormat() const override
    {
      return Player;
    }

    Container::Ptr Decode(const Binary::Container& rawData) const override
    {
      using namespace CompiledPT24;
      const Binary::View data(rawData);
      if (!Player->Match(data))
      {
        return {};
      }
      const auto& rawPlayer = *data.As<RawPlayer>();
      const uint_t dataAddr = rawPlayer.DataAddr;
      if (dataAddr < PLAYER_SIZE)
      {
        Dbg("Invalid compile addr");
        return {};
      }
      const auto modData = data.SubView(PLAYER_SIZE, MAX_MODULE_SIZE);
      const auto& rawHeader = *modData.As<RawHeader>();
      const uint_t patternsCount = GetPatternsCount(rawHeader, modData.Size());
      if (!patternsCount)
      {
        Dbg("Invalid patterns count");
        return {};
      }
      const uint_t compileAddr = dataAddr - PLAYER_SIZE;
      Dbg("Detected player compiled at #{:04x} with {} patterns", compileAddr, patternsCount);
      const auto builder = Formats::Chiptune::PatchedDataBuilder::Create(modData);
      // fix samples/ornaments offsets
      for (uint_t idx = offsetof(RawHeader, SamplesOffsets); idx != offsetof(RawHeader, PatternsOffset); idx += 2)
      {
        builder->FixLEWord(idx, -int_t(dataAddr));
      }
      // fix patterns offsets
      for (uint_t idx = rawHeader.PatternsOffset, lim = idx + 6 * patternsCount; idx != lim; idx += 2)
      {
        builder->FixLEWord(idx, -int_t(dataAddr));
      }
      const auto fixedModule = builder->GetResult();
      if (auto fixedParsed = Decoder->Decode(*fixedModule))
      {
        const auto totalSize = PLAYER_SIZE + fixedParsed->Size();
        return CreateContainer(std::move(fixedParsed), totalSize);
      }
      Dbg("Failed to parse fixed module");
      return {};
    }

  private:
    const Binary::Format::Ptr Player;
    const Formats::Chiptune::Decoder::Ptr Decoder;
  };

  Decoder::Ptr CreateCompiledPT24Decoder()
  {
    return MakePtr<CompiledPT24Decoder>();
  }
}  // namespace Formats::Packed
