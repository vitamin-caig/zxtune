/**
* 
* @file
*
* @brief  USF parser interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <formats/chiptune.h>

namespace Formats
{
  namespace Chiptune
  {
    namespace Ultra64SoundFormat
    {
      const uint_t VERSION_ID = 0x21;
      
      class Builder
      {
      public:
        virtual ~Builder() = default;
        
        virtual void SetRom(uint32_t offset, Binary::DataView content) = 0;
        virtual void SetSaveState(uint32_t offset, Binary::DataView content) = 0;
      };

      void ParseSection(const Binary::Container& data, Builder& target);
    }

    Decoder::Ptr CreateUSFDecoder();
  }
}
