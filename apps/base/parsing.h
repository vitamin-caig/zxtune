/*
Abstract:
  Parsing tools

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef BASE_PARSING_H_DEFINED
#define BASE_PARSING_H_DEFINED

//library includes
#include <parameters/modifier.h>

void ParseConfigFile(const String& filename, Parameters::Modifier& result);
//result will be overwritten
void ParseParametersString(const Parameters::NameType& prefix, const String& str, Parameters::Modifier& result);

#endif //BASE_PARSING_H_DEFINED
