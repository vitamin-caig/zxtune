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

// common includes
#include <types.h>

namespace Parameters
{
  class Accessor;
}

String GetModuleTitle(const String& format, const Parameters::Accessor& props);
