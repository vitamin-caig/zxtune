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

  class ProgressCallbackImpl : public ProgressCallback
  {
  public:
    ProgressCallbackImpl(uint_t total, const MessageData& baseData, const MessageReceiver& receiver)
      : Total(total)
      , Receiver(receiver)
      , Data(baseData)
    {
      Data.Progress = -1;
    }

    virtual void OnProgress(uint_t current)
    {
      const uint_t curProg = GetProgress(current);
      if (curProg != *Data.Progress)
      {
        Data.Progress = curProg;
        Receiver.ReportMessage(Data);
      }
    }

    virtual void OnProgress(uint_t current, const String& message)
    {
      const uint_t curProg = GetProgress(current);
      if (curProg != *Data.Progress)
      {
        Data.Progress = curProg;
        Data.Text = message;
        Receiver.ReportMessage(Data);
      }
    }
  private:
    uint_t GetProgress(uint_t position) const
    {
      return static_cast<uint_t>(uint64_t(position) * 100 / Total);
    }
  private:
    const uint_t Total;
    const MessageReceiver& Receiver;
    MessageData Data;
  };
}

namespace Log
{
  ProgressCallback::Ptr CreateProgressCallback(uint_t total, const Log::MessageData& baseData, const MessageReceiver& receiver)
  {
    return ProgressCallback::Ptr(new ProgressCallbackImpl(total, baseData, receiver));
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
