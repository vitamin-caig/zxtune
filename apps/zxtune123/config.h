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

#include "parameters/modifier.h"

#include "string_view.h"

void ParseConfigFile(StringView filename, Parameters::Modifier& result);
// result will be overwritten
void ParseParametersString(Parameters::Identifier prefix, StringView str, Parameters::Modifier& result);
