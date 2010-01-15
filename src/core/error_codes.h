/*
Abstract:
  Error codes for core subsystem

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef __CORE_ERROR_CODES_H_DEFINED__
#define __CORE_ERROR_CODES_H_DEFINED__

#include <error.h>

namespace ZXTune
{
  namespace Module
  {
    enum
    {
      ERROR_INVALID_PARAMETERS = Error::ModuleCode<'M', 'O', 'D'>::Value,
      ERROR_INVALID_FORMAT,
      ERROR_MODULE_END,
      ERROR_MODULE_CONVERT,
      ERROR_FIND_SUBMODULE,
      ERROR_DETECT_CANCELED,
      ERROR_FIND_CONTAINER_PLUGIN,
      ERROR_FIND_IMPLICIT_PLUGIN,
      ERROR_FIND_PLAYER_PLUGIN
    };
  }
}

#endif //__CORE_ERROR_CODES_H_DEFINED__
