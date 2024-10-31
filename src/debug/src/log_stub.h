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

namespace Debug
{
  inline void Log(const char* /*module*/, const char* /*msg*/) {}

  class Stream
  {
  public:
    explicit Stream(const char* /*module*/) {}

    template<class... P>
    void operator()(const char* /*msg*/, P&&... /*p*/) const
    {}
  };
}  // namespace Debug
