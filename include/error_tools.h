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

// common includes
#include <error.h>
// library includes
#include <strings/format.h>

//! @brief Building error object with formatted text
template<class S, class... P>
Error MakeFormattedError(Error::Location loc, S&& fmt, P&&... p)
{
  return Error(loc, Strings::Format(std::forward<S>(fmt), std::forward<P>(p)...));
}
