/*
Abstract:
  Logging functions interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef __LOGGING_H_DEFINED__
#define __LOGGING_H_DEFINED__

#include <string>
#ifdef UNICODE
#include <string_type.h>
#endif

#include <cassert>

#include <boost/format.hpp>

namespace Log
{
  bool IsDebuggingEnabled();

  void Message(const std::string& module, const std::string& msg);

  template<class T>
  inline const T& AdaptType(const T& param)
  {
    return param;
  }

  inline unsigned AdaptType(std::size_t param)
  {
    return static_cast<unsigned>(param);
  }

  #ifdef UNICODE
  inline const std::string& AdaptType(const String& param)
  {
    return std::string(param.begin(), param.end());
  }
  #endif

  inline void Debug(const std::string& module, const char* msg)
  {
    assert(msg);
    if (IsDebuggingEnabled())
    {
      Message(module, msg);
    }
  }

  template<class P1>
  inline void Debug(const std::string& module, const char* msg, const P1& p1)
  {
    assert(msg);
    if (IsDebuggingEnabled())
    {
      Message(module, (boost::format(msg) % AdaptType(p1)).str());
    }
  }
  
  template<class P1, class P2>
  inline void Debug(const std::string& module, const char* msg, const P1& p1, const P2& p2)
  {
    assert(msg);
    if (IsDebuggingEnabled())
    {
      Message(module, (boost::format(msg)
        % AdaptType(p1)
        % AdaptType(p2)).str());
    }
  }

  template<class P1, class P2, class P3>
  inline void Debug(const std::string& module, const char* msg, const P1& p1, const P2& p2, const P3& p3)
  {
    assert(msg);
    if (IsDebuggingEnabled())
    {
      Message(module, (boost::format(msg)
        % AdaptType(p1)
        % AdaptType(p2)
        % AdaptType(p3)).str());
    }
  }

  template<class P1, class P2, class P3, class P4>
  inline void Debug(const std::string& module, const char* msg, const P1& p1, const P2& p2, const P3& p3, const P4& p4)
  {
    assert(msg);
    if (IsDebuggingEnabled())
    {
      Message(module, (boost::format(msg)
        % AdaptType(p1)
        % AdaptType(p2)
        % AdaptType(p3)
        % AdaptType(p4)).str());
    }
  }

  template<class P1, class P2, class P3, class P4, class P5>
  inline void Debug(const std::string& module, const char* msg,
                    const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5)
  {
    assert(msg);
    if (IsDebuggingEnabled())
    {
      Message(module, (boost::format(msg)
        % AdaptType(p1)
        % AdaptType(p2)
        % AdaptType(p3)
        % AdaptType(p4)
        % AdaptType(p5)).str());
    }
  }
}
#endif //__LOGGING_H_DEFINED__
