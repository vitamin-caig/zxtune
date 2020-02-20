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
#include <binary/view.h>
#include <formats/chiptune.h>
#include <time/duration.h>

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

        virtual void SetTitle(String title) = 0;
        virtual void SetGame(String game) = 0;
        virtual void SetDumper(String dumper) = 0;
        virtual void SetComment(String comment) = 0;
        virtual void SetDumpDate(String date) = 0;
        virtual void SetIntro(Time::Milliseconds duration) = 0;
        virtual void SetLoop(Time::Milliseconds duration) = 0;
        virtual void SetFade(Time::Milliseconds duration) = 0;
        virtual void SetArtist(String artist) = 0;
        
        virtual void SetRAM(Binary::View data) = 0;
        virtual void SetDSPRegisters(Binary::View data) = 0;
        virtual void SetExtraRAM(Binary::View data) = 0;
      };

      Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
      Builder& GetStubBuilder();
    }

    Decoder::Ptr CreateSPCDecoder();
  }
}
