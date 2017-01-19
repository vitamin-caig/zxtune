/**
* 
* @file
*
* @brief  PSF related stuff implementation. Exe
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "psf_exe.h"
//common includes
#include <contract.h>
#include <make_ptr.h>
//library includes
#include <formats/chiptune/emulation/playstationsoundformat.h>
//std includes
#include <cstring>

namespace Module
{
  namespace PSF
  {
    class ExeParser : public Formats::Chiptune::PlaystationSoundFormat::Builder
    {
    public:
      explicit ExeParser(PsxExe& exe)
        : Exe(exe)
      {
      }

      void SetRegisters(uint32_t pc, uint32_t /*gp*/) override
      {
        if (pc && !Exe.PC)
        {
          Exe.PC = pc;
        }
      }

      void SetStackRegion(uint32_t head, uint32_t /*size*/) override
      {
        if (head && !Exe.SP)
        {
          Exe.SP = head;
        }
      }
      
      void SetRegion(String /*region*/, uint_t fps) override
      {
        if (fps && !Exe.RefreshRate)
        {
          Exe.RefreshRate = fps;
        }
      }
      
      void SetTextSection(uint32_t address, const Binary::Data& content) override
      {
        const auto addr = address & 0x1fffff;
        const auto size = content.Size();
        Require(addr >= 0x10000 && size <= 0x1f0000 && addr + size <= 0x200000);
        
        if (const auto oldSize = Exe.RAM.size())
        {
          const auto newAddr = std::min(addr, Exe.StartAddress);
          const auto newEnd = std::max(addr + size, Exe.StartAddress + oldSize);
          Exe.RAM.resize(newEnd - newAddr);
          if (newAddr < Exe.StartAddress)
          {
            std::memcpy(Exe.RAM.data() + (Exe.StartAddress - newAddr), Exe.RAM.data(), oldSize);
            Exe.StartAddress = newAddr;
          }
          std::memcpy(Exe.RAM.data() + (addr - newAddr), content.Start(), size);
        }
        else
        {
          const auto src = static_cast<const uint8_t*>(content.Start());
          Exe.RAM.assign(src, src + size);
          Exe.StartAddress = addr;
        }
      }
    private:
      PsxExe& Exe;
    };
    
    void PsxExe::Parse(const Binary::Container& data, PsxExe& exe)
    {
      ExeParser parser(exe);
      Formats::Chiptune::PlaystationSoundFormat::ParsePSXExe(data, parser);
    }
  }
}
