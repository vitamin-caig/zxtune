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

#include "parameters/identifier.h"

#include "string_view.h"

namespace Parameters
{
  class Modifier;
}  // namespace Parameters

void ParseConfigFile(StringView filename, Parameters::Modifier& result);
// result will be overwritten
void ParseParametersString(Parameters::Identifier prefix, StringView str, Parameters::Modifier& result);
