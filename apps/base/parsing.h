/*
Abstract:
  Parsing tools

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef BASE_PARSING_H_DEFINED
#define BASE_PARSING_H_DEFINED

//common includes
#include <error.h>
#include <parameters.h>

Error ParseConfigFile(const String& filename, Parameters::Map& result);
//result will be overwritten
Error ParseParametersString(const String& prefix, const String& str, Parameters::Map& result);

#endif //BASE_PARSING_H_DEFINED
