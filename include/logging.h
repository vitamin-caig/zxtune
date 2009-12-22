/*
Abstract:
  Logging functions interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef __LOG_H_DEFINED__
#define __LOG_H_DEFINED__

#include <string>
#include <boost/format.hpp>

namespace Log
{
  bool IsDebuggingEnabled();

  void Message(const std::string& module, const std::string& msg);

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
      Message(module, (boost::format(msg) % p1).str());
    }
  }
  
  template<class P1, class P2>
  inline void Debug(const std::string& module, const char* msg, const P1& p1, const P2& p2)
  {
    assert(msg);
    if (IsDebuggingEnabled())
    {
      Message(module, (boost::format(msg) % p1 % p2).str());
    }
  }

  template<class P1, class P2, class P3>
  inline void Debug(const std::string& module, const char* msg, const P1& p1, const P2& p2, const P3& p3)
  {
    assert(msg);
    if (IsDebuggingEnabled())
    {
      Message(module, (boost::format(msg) % p1 % p2 % p3).str());
    }
  }

  template<class P1, class P2, class P3, class P4>
  inline void Debug(const std::string& module, const char* msg, const P1& p1, const P2& p2, const P3& p3, const P4& p4)
  {
    assert(msg);
    if (IsDebuggingEnabled())
    {
      Message(module, (boost::format(msg) % p1 % p2 % p3 % p4).str());
    }
  }

  template<class P1, class P2, class P3, class P4, class P5>
  inline void Debug(const std::string& module, const char* msg, 
                    const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5)
  {
    assert(msg);
    if (IsDebuggingEnabled())
    {
      Message(module, (boost::format(msg) % p1 % p2 % p3 % p4 % p5).str());
    }
  }
}
#endif //__LOG_H_DEFINED__