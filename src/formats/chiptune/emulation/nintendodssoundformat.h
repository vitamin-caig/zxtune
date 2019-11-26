/**
* 
* @file
*
* @brief  2SF parser interface
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
    namespace NintendoDSSoundFormat
    {
      const uint_t VERSION_ID = 0x24;
      
      class Builder
      {
      public:
        virtual ~Builder() = default;
        
        virtual void SetChunk(uint32_t offset, Binary::DataView content) = 0;
      };

      void ParseRom(const Binary::Container& data, Builder& target);
      void ParseState(const Binary::Container& data, Builder& target);
    }

    Decoder::Ptr Create2SFDecoder();
  }
}
