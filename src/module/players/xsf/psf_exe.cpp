/**
 *
 * @file
 *
 * @brief  PSF related stuff implementation. Exe
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "module/players/xsf/psf_exe.h"
// common includes
#include <contract.h>
#include <make_ptr.h>
// library includes
#include <formats/chiptune/emulation/playstationsoundformat.h>

namespace Module::PSF
{
  class ExeParser : public Formats::Chiptune::PlaystationSoundFormat::Builder
  {
  public:
    explicit ExeParser(PsxExe& exe)
      : Exe(exe)
    {}

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

    void SetRegion(StringView /*region*/, uint_t fps) override
    {
      if (fps && !Exe.RefreshRate)
      {
        Exe.RefreshRate = fps;
      }
    }

    void SetTextSection(uint32_t address, Binary::View content) override
    {
      const auto addr = address & 0x1fffff;
      const auto size = content.Size();
      Require(addr >= 0x10000 && size <= 0x1f0000 && addr + size <= 0x200000);

      Exe.RAM.Update(addr, content);
    }

  private:
    PsxExe& Exe;
  };

  void PsxExe::Parse(Binary::View data, PsxExe& exe)
  {
    ExeParser parser(exe);
    Formats::Chiptune::PlaystationSoundFormat::ParsePSXExe(data, parser);
  }
}  // namespace Module::PSF
