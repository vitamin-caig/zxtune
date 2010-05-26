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

  // mask for all logging output
  const char DEBUG_ALL = '*';

  // helper singleton class to incapsulate logic
  class Logger
  {
  public:
    bool IsDebuggingEnabled(const std::string& module)
    {
      return Variable &&
        (*Variable == DEBUG_ALL || 0 == module.find(Variable));
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
      : Variable(::getenv(DEBUG_LOG_VARIABLE))
    {
    }
    
  private:
    const char* const Variable;
  };
}

namespace Log
{
  //public gate

  bool IsDebuggingEnabled(const std::string& module)
  {
    return Logger::Instance().IsDebuggingEnabled(module);
  }

  void Message(const std::string& module, const std::string& msg)
  {
    return Logger::Instance().Message(module, msg);
  }
}
