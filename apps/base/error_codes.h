/*
Abstract:
  Application error codes

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/
#ifndef BASE_ERROR_CODES_H_DEFINED
#define BASE_ERROR_CODES_H_DEFINED

#include <error.h>

enum ErrorCode
{
  NO_INPUT_FILES = Error::ModuleCode<'A', 'P', 'P'>::Value,
  INVALID_PARAMETER,
  NO_BACKENDS,
  UNKNOWN_ERROR,
  CONVERT_PARAMETERS,
  CONFIG_FILE
};

#endif //BASE_ERROR_CODES_H_DEFINED
