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

    template<class... P>
    void operator ()(const char* /*msg*/, P&&... /*p*/) const {}
  };
}
