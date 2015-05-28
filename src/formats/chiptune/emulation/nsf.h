/**
* 
* @file
*
* @brief  NSF support interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//common includes
#include <types.h>
//library includes
#include <formats/chiptune/polytrack_container.h>

namespace Formats
{
  namespace Chiptune
  {
    namespace NSF
    {
      PolyTrackContainer::Ptr Parse(const Binary::Container& data);
    }

    Decoder::Ptr CreateNSFDecoder();
  }
}
