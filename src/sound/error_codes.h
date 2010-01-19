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
      MIXER_UNSUPPORTED = Error::ModuleCode<'M', 'I', 'X'>::Value,
      MIXER_INVALID_PARAMETER,
      //filter-specific errors
      FILTER_INVALID_PARAMETER = Error::ModuleCode<'F', 'L', 'T'>::Value,
      //backend-specific errors
      BACKEND_NOT_FOUND = Error::ModuleCode<'B', 'N', 'D'>::Value,
      BACKEND_FAILED_CREATE,
      BACKEND_INVALID_PARAMETER,
      BACKEND_SETUP_ERROR,
      BACKEND_CONTROL_ERROR,
      BACKEND_PLATFORM_ERROR,
      BACKEND_UNSUPPORTED_FUNC
    };
  }
}

#endif //__SOUND_ERROR_CODES_H_DEFINED__
