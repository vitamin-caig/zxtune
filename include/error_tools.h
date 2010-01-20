/**
*
* @file     error_tools.h
* @brief    Error subsystem tools
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
* @see @ref Formatter type for format string specification
**/

#ifndef __ERROR_TOOLS_H_DEFINED__
#define __ERROR_TOOLS_H_DEFINED__

#include <error.h>
#include <formatter.h>

//! @brief Building error object with formatted text using up to 5 parameters
template<class P1>
inline Error MakeFormattedError(Error::LocationRef loc, Error::CodeType code, const String& fmt,
  const P1& p1)
{
  return Error(loc, code, (Formatter(fmt) % p1).str());
}

template<class P1, class P2>
inline Error MakeFormattedError(Error::LocationRef loc, Error::CodeType code, const String& fmt,
  const P1& p1, const P2& p2)
{
  return Error(loc, code, (Formatter(fmt) % p1 % p2).str());
}

template<class P1, class P2, class P3>
inline Error MakeFormattedError(Error::LocationRef loc, Error::CodeType code, const String& fmt,
  const P1& p1, const P2& p2, const P3& p3)
{
  return Error(loc, code, (Formatter(fmt) % p1 % p2 % p3).str());
}

template<class P1, class P2, class P3, class P4>
inline Error MakeFormattedError(Error::LocationRef loc, Error::CodeType code, const String& fmt,
  const P1& p1, const P2& p2, const P3& p3, const P4& p4)
{
  return Error(loc, code, (Formatter(fmt) % p1 % p2 % p3 % p4).str());
}

template<class P1, class P2, class P3, class P4, class P5>
inline Error MakeFormattedError(Error::LocationRef loc, Error::CodeType code, const String& fmt,
  const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5)
{
  return Error(loc, code, (Formatter(fmt) % p1 % p2 % p3 % p4 % p5).str());
}

#endif //__ERROR_TOOLS_H_DEFINED__
