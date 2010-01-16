/*
Abstract:
  Logging functions implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include <logging.h>

#include <cstdio>
#include <iostream>

namespace
{
  const char DEBUG_LOG_VARIABLE[] = "ZXTUNE_DEBUG_LOG";
  
  class Logger
  {
  public:
    bool IsDebuggingEnabled()
    {
      return Debugging;
    }
    
    void Message(const std::string& module, const std::string& msg)
    {
      std::cerr << '[' << module << "]: " << msg << std::endl;
    }
  
    static Logger& Instance()
    {
      static Logger instance;
      return instance;
    }
  private:
    Logger()
      : Debugging(0 != ::getenv(DEBUG_LOG_VARIABLE))
    {
    }
    
  private:
    const bool Debugging;
  };
}

namespace Log
{
  bool IsDebuggingEnabled()
  {
    return Logger::Instance().IsDebuggingEnabled();
  }

  void Message(const std::string& module, const std::string& msg)
  {
    return Logger::Instance().Message(module, msg);
  }
}
