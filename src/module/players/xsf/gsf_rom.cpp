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
        Rom.Content.Update(address & 0x1fffffff, content.Start(), content.Size());
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
