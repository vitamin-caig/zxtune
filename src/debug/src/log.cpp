/*
Abstract:
  Debug logging implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//library includes
#include <debug/log.h>
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
    bool EnabledFor(const std::string& module) const
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
  void Message(const std::string& module, const std::string& msg)
  {
    std::cerr << '[' << module << "]: " << msg << std::endl;
  }

  Stream::Stream(const std::string& module)
    : Module(module)
    , Enabled(DebugSwitch::Instance().EnabledFor(Module))
  {
  }
}
