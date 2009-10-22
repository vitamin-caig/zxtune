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
      ERROR_INVALID_FORMAT = Error::ModuleCode<'M', 'O'>::Value,
      ERROR_MODULE_END,
      ERROR_MODULE_OVERSEEK,
      ERROR_MODULE_CONVERT
    };
  }
}

#endif //__CORE_ERROR_CODES_H_DEFINED__
