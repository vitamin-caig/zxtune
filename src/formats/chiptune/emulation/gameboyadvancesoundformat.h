/**
* 
* @file
*
* @brief  GSF parser interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <binary/data_view.h>
#include <formats/chiptune.h>

namespace Formats
{
  namespace Chiptune
  {
    namespace GameBoyAdvanceSoundFormat
    {
      const uint_t VERSION_ID = 0x22;
      
      class Builder
      {
      public:
        virtual ~Builder() = default;
        
        virtual void SetEntryPoint(uint32_t addr) = 0;
        virtual void SetRom(uint32_t addr, Binary::DataView content) = 0;
      };

      void ParseRom(Binary::DataView data, Builder& target);
    }

    Decoder::Ptr CreateGSFDecoder();
  }
}
