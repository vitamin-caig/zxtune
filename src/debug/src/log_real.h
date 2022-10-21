/**
 *
 * @file
 *
 * @brief  Debug logging implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <types.h>
// std includes
#include <cassert>
#include <string>
#include <utility>
// library includes
#include <strings/format.h>

namespace Debug
{
  //! @brief Unconditionally outputs debug message
  void Message(const String& module, const String& msg);

  //! @brief Checks if debug logs are enabled for module
  bool IsEnabledFor(const String& module);

  inline void Log(const String& module, const String& msg)
  {
    if (IsEnabledFor(module))
    {
      Message(module, msg);
    }
  }

  /*
     @brief Per-module debug stream
     @code
       const Debug::Stream Dbg(THIS_MODULE);
       ...
       Dbg("message {}", parameter);
     @endcode
  */
  class Stream
  {
  public:
    explicit Stream(const char* module)
      : Module(module)
      , Enabled(IsEnabledFor(Module))
    {}

    //! @brief Conditionally outputs debug message from specified module
    void operator()(const char* msg) const
    {
      assert(msg);
      if (Enabled)
      {
        Message(Module, msg);
      }
    }

    //! @brief Conditionally outputs formatted debug message from specified module
    template<class... P>
    void operator()(const char* msg, P&&... p) const
    {
      assert(msg);
      if (Enabled)
      {
        Message(Module, Strings::Format(msg, std::forward<P>(p)...));
      }
    }

  private:
    const String Module;
    const bool Enabled;
  };
}  // namespace Debug
