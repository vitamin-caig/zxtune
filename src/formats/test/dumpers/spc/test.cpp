/**
 *
 * @file
 *
 * @brief  SPC dumper
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "../../utils.h"
#include <formats/chiptune/emulation/spc.h>
#include <strings/format.h>
#include <time/serialize.h>

namespace
{
  using namespace Formats::Chiptune;

  class SpcBuilder : public SPC::Builder
  {
  public:
    void SetRegisters(uint16_t pc, uint8_t a, uint8_t x, uint8_t y, uint8_t psw, uint8_t sp) override
    {
      std::cout << Strings::Format("Registers: PC=0x{:04x} A=0x{:02x} X=0x{:02x} Y=0x{:02x} PSW=0x{:02x} SP=0x{:02x}\n",
                                   uint_t(pc), uint_t(a), uint_t(x), uint_t(y), uint_t(psw), uint_t(sp));
    }

    void SetTitle(String title) override
    {
      std::cout << "Title: " << title << std::endl;
    }

    void SetGame(String game) override
    {
      std::cout << "Game: " << game << std::endl;
    }

    void SetDumper(String dumper) override
    {
      std::cout << "Dumper: " << dumper << std::endl;
    }

    void SetComment(String comment) override
    {
      std::cout << "Comment: " << comment << std::endl;
    }

    void SetDumpDate(String date) override
    {
      std::cout << "Dump date: " << date << std::endl;
    }

    void SetIntro(Time::Milliseconds duration) override
    {
      std::cout << "Intro: " << Time::ToString(duration) << std::endl;
    }

    void SetLoop(Time::Milliseconds duration) override
    {
      std::cout << "Loop: " << Time::ToString(duration) << std::endl;
    }

    void SetFade(Time::Milliseconds duration) override
    {
      std::cout << "Fade: " << Time::ToString(duration) << std::endl;
    }

    void SetArtist(String artist) override
    {
      std::cout << "Artist: " << artist << std::endl;
    }

    void SetRAM(Binary::View data) override
    {
      std::cout << "RAM,bytes: " << data.Size() << std::endl;
    }

    void SetDSPRegisters(Binary::View data) override
    {
      std::cout << "DSP registers,bytes: " << data.Size() << std::endl;
    }

    void SetExtraRAM(Binary::View data) override
    {
      std::cout << "ExtraRAM,bytes: " << data.Size() << std::endl;
    }
  };
}  // namespace

int main(int argc, char* argv[])
{
  try
  {
    if (argc < 2)
    {
      return 0;
    }
    std::unique_ptr<Binary::Dump> rawData(new Binary::Dump());
    Test::OpenFile(argv[1], *rawData);
    const Binary::Container::Ptr data = Binary::CreateContainer(std::move(rawData));
    SpcBuilder builder;
    if (const auto result = Formats::Chiptune::SPC::Parse(*data, builder))
    {
      std::cout << result->Size() << " bytes total" << std::endl;
    }
    else
    {
      std::cout << "Failed to parse" << std::endl;
    }
    return 0;
  }
  catch (const std::exception& e)
  {
    std::cout << e.what() << std::endl;
    return 1;
  }
}
