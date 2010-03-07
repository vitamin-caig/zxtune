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

#include <parameters.h>

class Error;

Error ParseConfigFile(const String& filename, Parameters::Map& result);
//result will be overwritten
Error ParseParametersString(const String& prefix, const String& str, Parameters::Map& result);
//format time
String UnparseFrameTime(uint_t timeInFrames, uint_t frameDurationMicrosec);

#endif //BASE_PARSING_H_DEFINED
