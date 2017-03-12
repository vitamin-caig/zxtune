/**
* 
* @file
*
* @brief  SPC support interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//common includes
#include <types.h>
//library includes
#include <formats/chiptune.h>
#include <time/stamp.h>

namespace Formats
{
  namespace Chiptune
  {
    namespace SPC
    {
      class Builder
      {
      public:
        virtual ~Builder() = default;

        virtual void SetRegisters(uint16_t pc, uint8_t a, uint8_t x, uint8_t y, uint8_t psw, uint8_t sp) = 0;

        virtual void SetTitle(const String& title) = 0;
        virtual void SetGame(const String& game) = 0;
        virtual void SetDumper(const String& dumper) = 0;
        virtual void SetComment(const String& comment) = 0;
        virtual void SetDumpDate(const String& date) = 0;
        virtual void SetIntro(Time::Milliseconds duration) = 0;
        virtual void SetLoop(Time::Milliseconds duration) = 0;
        virtual void SetFade(Time::Milliseconds duration) = 0;
        virtual void SetArtist(const String& artist) = 0;
        
        virtual void SetRAM(const void* data, std::size_t size) = 0;
        virtual void SetDSPRegisters(const void* data, std::size_t size) = 0;
        virtual void SetExtraRAM(const void* data, std::size_t size) = 0;
      };

      Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
      Builder& GetStubBuilder();
    }

    Decoder::Ptr CreateSPCDecoder();
  }
}
