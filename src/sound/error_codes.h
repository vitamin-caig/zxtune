/*
Abstract:
  Sound subsystem error codes definitions

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef __SOUND_ERROR_CODES_H_DEFINED__
#define __SOUND_ERROR_CODES_H_DEFINED__

#include <error.h>

namespace ZXTune
{
  namespace Sound
  {
    enum
    {
      MIXER_CHANNELS_MISMATCH = Error::ModuleCode<'M', 'X'>::Value,
      MIXER_UNSUPPORTED
    };
  }
}

#endif //__SOUND_ERROR_CODES_H_DEFINED__
