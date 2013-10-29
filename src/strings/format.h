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
//boost includes
#include <boost/format.hpp>

namespace Strings
{
  //! @brief Format functions
  //! @code
  //! const String& txt = Strings::Format(FORMAT_STRING, param1, param2);
  //! @endcode
  //! <A HREF="http://www.boost.org/doc/libs/1_41_0/libs/format/doc/format.html#syntax">Syntax of format string</A>

  template<class P1>
  String Format(const String& format, const P1& p1)
  {
    return (boost::basic_format<Char>(format) % p1).str();
  }

  template<class P1, class P2>
  String Format(const String& format, const P1& p1, const P2& p2)
  {
    return (boost::basic_format<Char>(format) % p1 % p2).str();
  }

  template<class P1, class P2, class P3>
  String Format(const String& format, const P1& p1, const P2& p2, const P3& p3)
  {
    return (boost::basic_format<Char>(format) % p1 % p2 % p3).str();
  }

  template<class P1, class P2, class P3, class P4>
  String Format(const String& format, const P1& p1, const P2& p2, const P3& p3, const P4& p4)
  {
    return (boost::basic_format<Char>(format) % p1 % p2 % p3 % p4).str();
  }

  template<class P1, class P2, class P3, class P4, class P5>
  String Format(const String& format, const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5)
  {
    return (boost::basic_format<Char>(format) % p1 % p2 % p3 % p4 % p5).str();
  }

  template<class P1, class P2, class P3, class P4, class P5, class P6>
  String Format(const String& format, const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5, const P6& p6)
  {
    return (boost::basic_format<Char>(format) % p1 % p2 % p3 % p4 % p5 % p6).str();
  }

  String FormatTime(uint_t hours, uint_t minutes, uint_t seconds, uint_t frames);
}
