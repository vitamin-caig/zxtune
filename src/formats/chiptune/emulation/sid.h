/**
* 
* @file
*
* @brief  SID support interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//common includes
#include <types.h>
//library includes
#include <formats/chiptune.h>

namespace Formats
{
  namespace Chiptune
  {
    namespace SID
    {
      uint_t GetModulesCount(Binary::DataView data);

      /*
       * Since md5 digest of sid modules depends on tracks count, it's impossible to fix it
       * (at least without length database renewal). This container plugin can be applied only once while search
       * and only for multitrack modules.
       */
      Binary::Container::Ptr FixStartSong(Binary::DataView data, uint_t idx);
    }

    Decoder::Ptr CreateSIDDecoder();
  }
}
