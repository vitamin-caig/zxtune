/**
 *
 * @file
 *
 * @brief  %Error subsystem tools
 *
 * @author vitamin.caig@gmail.com
 *
 * @see #Formatter type for format string specification
 *
 **/

#pragma once

#include "strings/format.h"

#include "error.h"  // IWYU pragma: export

//! @brief Building error object with formatted text
template<class S, class... P>
Error MakeFormattedError(Error::Location loc, S&& fmt, P&&... p)
{
  return Error(loc, Strings::FormatRuntime(std::forward<S>(fmt), std::forward<P>(p)...));
}
