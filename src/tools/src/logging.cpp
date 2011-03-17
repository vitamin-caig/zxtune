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
  using namespace Log;

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

  class ProcentProgressCallback : public ProgressCallback
  {
  public:
    ProcentProgressCallback(uint_t total, ProgressCallback& delegate)
      : Total(total)
      , Delegate(delegate)
      , Current(-1)
    {
    }

    virtual void OnProgress(uint_t current)
    {
      const int_t curProg = ScaleToProgress(current);
      if (curProg != Current)
      {
        Current = curProg;
        Delegate.OnProgress(static_cast<uint_t>(Current));
      }
    }

    virtual void OnProgress(uint_t current, const String& message)
    {
      const int_t curProg = ScaleToProgress(current);
      if (curProg != Current)
      {
        Current = curProg;
        Delegate.OnProgress(static_cast<uint_t>(Current), message);
      }
    }
  private:
    int_t ScaleToProgress(uint_t position) const
    {
      return static_cast<uint_t>(uint64_t(position) * 100 / Total);
    }
  private:
    const uint_t Total;
    ProgressCallback& Delegate;
    int_t Current;
  };
}

namespace Log
{
  ProgressCallback::Ptr CreatePercentProgressCallback(uint_t total, ProgressCallback& delegate)
  {
    return ProgressCallback::Ptr(new ProcentProgressCallback(total, delegate));
  }

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
