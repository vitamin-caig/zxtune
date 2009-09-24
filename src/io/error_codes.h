/*
Abstract:
  Error codes for IO subsystem

Last changed:
  $Id: $

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
    enum Error
    {
      SUCCESS = Error::ModuleCode<'I', 'O'>::Value,
      NOT_FOUND,
      CANCELLED,
      NO_ACCESS,
      IO_ERROR,
    };
  }
}

#endif //__IO_ERROR_CODES_H_DEFINED__
