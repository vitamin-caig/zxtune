/*
Abstract:
  Error codes for IO subsystem

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef __IO_ERROR_CODES_H_DEFINED__
#define __IO_ERROR_CODES_H_DEFINED__

#include <error.h>

namespace ZXTune
{
  namespace IO
  {
    enum
    {
      NOT_SUPPORTED = Error::ModuleCode<'I', 'O', 'S'>::Value,
      NOT_OPENED,
      NOT_FOUND,
      CANCELLED,
      NO_ACCESS,
      IO_ERROR,
    };
  }
}

#endif //__IO_ERROR_CODES_H_DEFINED__
