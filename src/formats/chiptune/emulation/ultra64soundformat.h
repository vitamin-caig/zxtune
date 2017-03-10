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
      
      //TODO: add SR64 parser
    }

    Decoder::Ptr CreateUSFDecoder();
  }
}
