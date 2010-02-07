/*
Abstract:
  Application error codes

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
  
  This file is a part of zxtune123 application based on zxtune library
*/
#ifndef ZXTUNE123_ERROR_CODES_H_DEFINED
#define ZXTUNE123_ERROR_CODES_H_DEFINED

#include <error.h>

const std::string THIS_MODULE("zxtune123");

enum ErrorCode
{
  NO_INPUT_FILES = Error::ModuleCode<'Z', 'X', 'T'>::Value,
  INVALID_PARAMETER,
  NO_BACKENDS,
  UNKNOWN_ERROR,
  CONVERT_PARAMETERS,
  CONFIG_FILE
};

#endif //ZXTUNE123_ERROR_CODES_H_DEFINED
