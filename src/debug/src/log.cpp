/**
* 
* @file
*
* @brief  Debug logging implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "log_real.h"
//std includes
#include <cstdio>
#include <cstring>
#include <iostream>

namespace
{
  // environment variable used to switch on logging
  const char DEBUG_LOG_VARIABLE[] = "ZXTUNE_DEBUG_LOG";

  // mask for all logging output
  const char DEBUG_ALL = '*';

  class DebugSwitch
  {
    DebugSwitch()
      : Variable(::getenv(DEBUG_LOG_VARIABLE))
      , VariableSize(Variable ? std::strlen(Variable) : 0)
    {
    }
  public:
    bool EnabledFor(const String& module) const
    {
      return Variable &&
        (*Variable == DEBUG_ALL || 0 == module.compare(0, VariableSize, Variable));
    }

    static DebugSwitch& Instance()
    {
      static DebugSwitch self;
      return self;
    }
  private:
    const char* const Variable;
    const std::size_t VariableSize;
  };
}

namespace Debug
{
  void Message(const String& module, const String& msg)
  {
    std::cerr << '[' << module << "]: " << msg << std::endl;
  }

  bool IsEnabledFor(const String& module)
  {
    return DebugSwitch::Instance().EnabledFor(module);
  }
}
