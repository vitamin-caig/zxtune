/*
Abstract:
  Logging functions implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//common includes
#include <logging.h>
//std includes
#include <cstdio>
#include <iostream>

namespace
{
  // environment variable used to switch on logging
  const char DEBUG_LOG_VARIABLE[] = "ZXTUNE_DEBUG_LOG";

  const char DEBUG_ALL = '*';

  class Logger
  {
  public:
    bool IsDebuggingEnabled()
    {
      return Debugging;
    }
    
    void Message(const std::string& module, const std::string& msg)
    {
      if (Variable &&
          (*Variable == DEBUG_ALL ||
           module == Variable))
      {
        std::cerr << '[' << module << "]: " << msg << std::endl;
      }
    }
  
    static Logger& Instance()
    {
      static Logger instance;
      return instance;
    }
  private:
    Logger()
      : Variable(::getenv(DEBUG_LOG_VARIABLE))
      , Debugging(0 != Variable)
    {
    }
    
  private:
    const char* Variable;
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
