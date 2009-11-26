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
      //mixer-specific errors
      MIXER_UNSUPPORTED = Error::ModuleCode<'M', 'X'>::Value,
      MIXER_INVALID_MATRIX,
      //filter-specific errors
      FILTER_INVALID_PARAMS = Error::ModuleCode<'F', 'L'>::Value,
      //backend-specific errors
      BACKEND_NOT_FOUND = Error::ModuleCode<'B', 'D'>::Value,
      BACKEND_FAILED_CREATE,
    };
  }
}

#endif //__SOUND_ERROR_CODES_H_DEFINED__
