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

#include "strings/format.h"

#include "string_view.h"
#include "types.h"

#include <cassert>
#include <utility>

namespace Debug
{
  //! @brief Unconditionally outputs debug message
  void Message(StringView module, StringView msg);

  //! @brief Checks if debug logs are enabled for module
  bool IsEnabledFor(StringView module);

  inline void Log(StringView module, StringView msg)
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
    explicit Stream(StringView module)
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
    constexpr void operator()(Strings::FormatString<P...> msg, P&&... p) const
    {
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
