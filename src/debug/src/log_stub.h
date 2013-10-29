/**
*
* @file
*
* @brief  Debug logging stub implementation
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//std includes
#include <string>

namespace Debug
{
  class Stream
  {
  public:
    explicit Stream(const char* /*module*/) {}

    void operator ()(const char* /*msg*/) const {}

    template<class P1>
    void operator ()(const char* /*msg*/, const P1& /*p1*/) const {}

    template<class P1, class P2>
    void operator ()(const char* /*msg*/, const P1& /*p1*/, const P2& /*p2*/) const {}

    template<class P1, class P2, class P3>
    void operator ()(const char* /*msg*/, const P1& /*p1*/, const P2& /*p2*/, const P3& /*p3*/) const {}

    template<class P1, class P2, class P3, class P4>
    void operator ()(const char* /*msg*/, const P1& /*p1*/, const P2& /*p2*/, const P3& /*p3*/, const P4& /*p4*/) const {}

    template<class P1, class P2, class P3, class P4, class P5>
    void operator ()(const char* /*msg*/, const P1& /*p1*/, const P2& /*p2*/, const P3& /*p3*/, const P4& /*p4*/, const P5& /*p5*/) const {}
  };
}
