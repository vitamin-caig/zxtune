/**
 *
 * @file
 *
 * @brief  Debug logging implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "log_real.h"

#include "string_view.h"

#include <cstdio>
#include <cstring>
#include <iostream>

namespace
{
  // environment variable used to switch on logging
  const char DEBUG_LOG_VARIABLE[] = "ZXTUNE_DEBUG_LOG";

  StringView GetDebugLogVariable()
  {
    if (const auto* str = ::getenv(DEBUG_LOG_VARIABLE))
    {
      return {str};
    }
    else
    {
      return {};
    }
  }

  // mask for all logging output
  const char DEBUG_ALL = '*';

  class DebugSwitch
  {
    DebugSwitch()
      : Variable(GetDebugLogVariable())
    {}

  public:
    bool EnabledFor(StringView module) const
    {
      return !Variable.empty() && (Variable[0] == DEBUG_ALL || module.starts_with(Variable));
    }

    static DebugSwitch& Instance()
    {
      static DebugSwitch self;
      return self;
    }

  private:
    const StringView Variable;
  };
}  // namespace

namespace Debug
{
  void Message(StringView module, StringView msg)
  {
    std::cerr << '[' << module << "]: " << msg << std::endl;
  }

  bool IsEnabledFor(StringView module)
  {
    return DebugSwitch::Instance().EnabledFor(module);
  }
}  // namespace Debug
