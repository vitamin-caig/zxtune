/*
Abstract:
  Parsing tools

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
  
  This file is a part of zxtune123 application based on zxtune library
*/
#ifndef ZXTUNE123_PARSING_H_DEFINED
#define ZXTUNE123_PARSING_H_DEFINED

#include <parameters.h>

class Error;

//result will be overwritten
Error ParseParametersString(const String& prefix, const String& str, Parameters::Map& result);
//format time
String UnparseFrameTime(unsigned timeInFrames, unsigned frameDurationMicrosec);

#endif //ZXTUNE123_PARSING_H_DEFINED
