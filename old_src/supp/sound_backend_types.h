#ifndef __SOUND_BACKEND_TYPES_H_DEFINED__
#define __SOUND_BACKEND_TYPES_H_DEFINED__

#include "sound_backend.h"

namespace ZXTune
{
  namespace Sound
  {
    /// Driver flags
    const unsigned BUFFER_DEPTH_MASK = 7;//up to 7 levels depth
    const unsigned MIN_BUFFER_DEPTH = 2;
    const unsigned MAX_BUFFER_DEPTH = 7;
    //for file backend
    const unsigned RAW_STREAM = 8;
    const unsigned ANNOTATE_STREAM = 16;
  }
}


#endif //__SOUND_BACKEND_TYPES_H_DEFINED__
