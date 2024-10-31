/**
 *
 * @file
 *
 * @brief  GSF related stuff implementation. Rom
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/players/xsf/gsf_rom.h"

#include "formats/chiptune/emulation/gameboyadvancesoundformat.h"

#include "contract.h"
#include "make_ptr.h"

namespace Module::GSF
{
  // Some details at https://github.com/loveemu/gsfopt/blob/master/src/gsfopt.cpp
  class RomParser : public Formats::Chiptune::GameBoyAdvanceSoundFormat::Builder
  {
  public:
    explicit RomParser(GbaRom& rom)
      : Rom(rom)
    {}

    void SetEntryPoint(uint32_t addr) override
    {
      if (addr)
      {
        addr = FixEntrypoint(addr);
        Require(addr == EWRAM_START || addr == PAKROM_START);
        if (!Rom.EntryPoint)
        {
          Rom.EntryPoint = addr;
        }
        else
        {
          Require(Rom.EntryPoint == addr);
        }
      }
    }

    void SetRom(uint32_t address, Binary::View content) override
    {
      const auto entryArea = GetArea(Rom.EntryPoint);
      Require(entryArea != Area::Unknown);
      const auto realAddress = Rom.EntryPoint + (address & static_cast<uint32_t>(entryArea));
      Rom.Content.Update(realAddress, content);
    }

  private:
    // https://www.coranac.com/tonc/text/hardware.htm
    static constexpr uint32_t EWRAM_START = 0x02000000;
    static constexpr uint32_t EWRAM_SIZE = 0x40000;
    static constexpr uint32_t PAKROM_START = 0x08000000;
    static constexpr uint32_t PAKROM_SIZE = 0x02000000;

    static uint32_t FixEntrypoint(uint32_t addr)
    {
      switch (addr)
      {
      case 0x80000000:  // Crash Bandicoot 2
        return PAKROM_START;
      default:
        return addr;
      }
    }

    enum class Area : uint32_t
    {
      Unknown,
      Multiboot = EWRAM_SIZE - 1,
      Rom = PAKROM_SIZE - 1
    };

    static Area GetArea(uint32_t addr)
    {
      if (addr >= EWRAM_START && addr < EWRAM_START + EWRAM_SIZE)
      {
        return Area::Multiboot;
      }
      else if (addr >= PAKROM_START && addr < PAKROM_START + PAKROM_SIZE)
      {
        return Area::Rom;
      }
      else
      {
        return Area::Unknown;
      }
    }

  private:
    GbaRom& Rom;
  };

  void GbaRom::Parse(Binary::View data, GbaRom& rom)
  {
    RomParser parser(rom);
    Formats::Chiptune::GameBoyAdvanceSoundFormat::ParseRom(data, parser);
  }
}  // namespace Module::GSF
