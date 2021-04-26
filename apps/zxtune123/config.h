/**
 *
 * @file
 *
 * @brief  Parsing tools
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// library includes
#include <parameters/modifier.h>

void ParseConfigFile(const String& filename, Parameters::Modifier& result);
// result will be overwritten
void ParseParametersString(Parameters::Identifier prefix, const String& str, Parameters::Modifier& result);
