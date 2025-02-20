/**
 *
 * @file
 *
 * @brief Format tools interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "string_view.h"
#include "types.h"

namespace Parameters
{
  class Accessor;
}

String GetModuleTitle(StringView format, const Parameters::Accessor& props);
