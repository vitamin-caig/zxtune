/**
*
* @file
*
* @brief  Formatter type definition
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//common includes
#include <types.h>
//std includes
#include <utility>
//boost includes
#include <boost/format.hpp>

namespace Strings
{
  namespace Details
  {
    template<class F>
    F DoFormat(F&& fmt)
    {
      return fmt;
    }
  
    template<class F, class P, class... R>
    F DoFormat(F&& fmt, P&& p, R&&... rest)
    {
      fmt % std::forward<P>(p);
      return DoFormat(std::forward<F>(fmt), rest...);
    }
  }
  //! @brief Format functions
  //! @code
  //! const String& txt = Strings::Format(FORMAT_STRING, param1, param2);
  //! @endcode
  //! <A HREF="http://www.boost.org/doc/libs/1_41_0/libs/format/doc/format.html#syntax">Syntax of format string</A>
  template<class S, class... P>
  String Format(S&& format, P&&... params)
  {
    return Details::DoFormat(boost::basic_format<Char>(format), params...).str();
  }

  String FormatTime(uint_t hours, uint_t minutes, uint_t seconds, uint_t frames);
}
