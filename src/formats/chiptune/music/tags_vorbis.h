/**
*
* @file
*
* @brief  Vorbis tags parser interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "formats/chiptune/builder_meta.h"
//library includes
#include <binary/input_stream.h>

namespace Formats
{
  namespace Chiptune
  {
    namespace Vorbis
    {
      void ParseComment(Binary::DataInputStream& stream, MetaBuilder& target);
    }
  }
}
