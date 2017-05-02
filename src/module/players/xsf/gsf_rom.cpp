/**
* 
* @file
*
* @brief  GSF related stuff implementation. Rom
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "gsf_rom.h"
//common includes
#include <contract.h>
#include <make_ptr.h>
//library includes
#include <formats/chiptune/emulation/gameboyadvancesoundformat.h>
//std includes
#include <cstring>

namespace Module
{
  namespace GSF
  {
    class RomParser : public Formats::Chiptune::GameBoyAdvanceSoundFormat::Builder
    {
    public:
      explicit RomParser(GbaRom& rom)
        : Rom(rom)
      {
      }

      void SetEntryPoint(uint32_t addr) override
      {
        if (addr && !Rom.EntryPoint)
        {
          Rom.EntryPoint = addr;
        }
      }
      
      void SetRom(uint32_t address, const Binary::Data& content) override
      {
        const auto addr = address & 0x1fffffff;
        const auto size = content.Size();

        if (const auto oldSize = Rom.Data.size())
        {
          const auto newAddr = std::min(addr, Rom.Start);
          const auto newEnd = std::max(addr + size, Rom.Start + oldSize);
          Rom.Data.resize(newEnd - newAddr);
          if (newAddr < Rom.Start)
          {
            std::memcpy(Rom.Data.data() + (Rom.Start - newAddr), Rom.Data.data(), oldSize);
            Rom.Start = newAddr;
          }
          std::memcpy(Rom.Data.data() + (addr - newAddr), content.Start(), size);
        }
        else
        {
          const auto src = static_cast<const uint8_t*>(content.Start());
          Rom.Data.assign(src, src + size);
          Rom.Start = addr;
        }
      }
    private:
      GbaRom& Rom;
    };
    
    void GbaRom::Parse(const Binary::Container& data, GbaRom& rom)
    {
      RomParser parser(rom);
      Formats::Chiptune::GameBoyAdvanceSoundFormat::ParseRom(data, parser);
    }
  }
}
