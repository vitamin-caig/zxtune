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

namespace
{
  using namespace Formats::Chiptune;

   class SpcBuilder : public SPC::Builder
   {
   public:
     void SetRegisters(uint16_t pc, uint8_t a, uint8_t x, uint8_t y, uint8_t psw, uint8_t sp) override
     {
       std::cout << Strings::Format("Registers: PC=0x%1$04x A=0x%2$02x X=0x%3$02x Y=0x%3$02x PSW=0x%3$02x SP=0x%3$02x\n",
         uint_t(pc), uint_t(a), uint_t(x), uint_t(y), uint_t(psw), uint_t(sp));
     }

     void SetTitle(const String& title) override
     {
       std::cout << "Title: " << title << std::endl;
     }

     void SetGame(const String& game) override
     {
       std::cout << "Game: " << game << std::endl;
     }

     void SetDumper(const String& dumper) override
     {
       std::cout << "Dumper: " << dumper << std::endl;
     }

     void SetComment(const String& comment) override
     {
       std::cout << "Comment: " << comment << std::endl;
     }

     void SetDumpDate(const String& date) override
     {
       std::cout << "Dump date: " << date << std::endl;
     }

     void SetIntro(Time::Milliseconds duration) override
     {
       std::cout << "Intro,ms: " << duration.Get() << std::endl;
     }

     void SetLoop(Time::Milliseconds duration) override
     {
       std::cout << "Duration,ms: " << duration.Get() << std::endl;
     }

     void SetFade(Time::Milliseconds duration) override
     {
       std::cout << "Fade,ms: " << duration.Get() << std::endl;
     }

     void SetArtist(const String& artist) override
     {
       std::cout << "Artist: " << artist << std::endl;
     }

     void SetRAM(const void* data, std::size_t size) override
     {
       std::cout << "RAM,bytes: " << size << std::endl;
     }

     void SetDSPRegisters(const void* data, std::size_t size) override
     {
       std::cout << "DSP registers,bytes: " << size << std::endl;
     }

     void SetExtraRAM(const void* data, std::size_t size) override
     {
       std::cout << "ExtraRAM,bytes: " << size << std::endl;
     }
   };
}

int main(int argc, char* argv[])
{
  try
  {
    if (argc < 2)
    {
      return 0;
    }
    std::unique_ptr<Dump> rawData(new Dump());
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
