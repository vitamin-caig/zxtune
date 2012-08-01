/**
*
* @file     logging.h
* @brief    Logging functions interface
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef __LOGGING_H_DEFINED__
#define __LOGGING_H_DEFINED__

//common includes
#include <types.h>
//std includes
#include <cassert>
#include <string>
#include <memory>
//boost includes
#include <boost/format.hpp>
#include <boost/optional.hpp>

//! @brief Namespace is used for logging and other informational purposes
namespace Log
{
  //! @brief Progress receiver
  class ProgressCallback
  {
  public:
    typedef std::auto_ptr<ProgressCallback> Ptr;
    virtual ~ProgressCallback() {}
    virtual void OnProgress(uint_t current) = 0;
    virtual void OnProgress(uint_t current, const String& message) = 0;

    static ProgressCallback& Stub();
  };

  ProgressCallback::Ptr CreatePercentProgressCallback(uint_t total, ProgressCallback& delegate);
  ProgressCallback::Ptr CreateNestedPercentProgressCallback(uint_t total, uint_t current, ProgressCallback& delegate);

  //! @brief Checks if debugging messages output for specified module is enabled
  bool IsDebuggingEnabled(const std::string& module);

  //! @brief Unconditionally outputs debug message
  void Message(const std::string& module, const std::string& msg);

  // Type adapters template for std::string output
  template<class T>
  inline const T& AdaptType(const T& param)
  {
    return param;
  }

  #ifdef UNICODE
  // Unicode adapter
  inline std::string AdaptType(const String& param)
  {
    return ToStdString(param);
  }
  #endif

  //! @brief Conditionally outputs debug message from specified module
  inline void Debug(const std::string& module, const char* msg)
  {
    assert(msg);
    if (IsDebuggingEnabled(module))
    {
      Message(module, msg);
    }
  }

  //! @brief Conditionally outputs formatted (up to 5 parameters) debug message from specified module
  //! @note Template functions and public level checking due to performance issues
  template<class P1>
  inline void Debug(const std::string& module, const char* msg, const P1& p1)
  {
    assert(msg);
    if (IsDebuggingEnabled(module))
    {
      Message(module, (boost::format(msg)
        % AdaptType(p1))
        .str());
    }
  }

  template<class P1, class P2>
  inline void Debug(const std::string& module, const char* msg, const P1& p1, const P2& p2)
  {
    assert(msg);
    if (IsDebuggingEnabled(module))
    {
      Message(module, (boost::format(msg)
        % AdaptType(p1)
        % AdaptType(p2))
        .str());
    }
  }

  template<class P1, class P2, class P3>
  inline void Debug(const std::string& module, const char* msg, const P1& p1, const P2& p2, const P3& p3)
  {
    assert(msg);
    if (IsDebuggingEnabled(module))
    {
      Message(module, (boost::format(msg)
        % AdaptType(p1)
        % AdaptType(p2)
        % AdaptType(p3))
        .str());
    }
  }

  template<class P1, class P2, class P3, class P4>
  inline void Debug(const std::string& module, const char* msg, const P1& p1, const P2& p2, const P3& p3, const P4& p4)
  {
    assert(msg);
    if (IsDebuggingEnabled(module))
    {
      Message(module, (boost::format(msg)
        % AdaptType(p1)
        % AdaptType(p2)
        % AdaptType(p3)
        % AdaptType(p4))
        .str());
    }
  }

  template<class P1, class P2, class P3, class P4, class P5>
  inline void Debug(const std::string& module, const char* msg,
                    const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5)
  {
    assert(msg);
    if (IsDebuggingEnabled(module))
    {
      Message(module, (boost::format(msg)
        % AdaptType(p1)
        % AdaptType(p2)
        % AdaptType(p3)
        % AdaptType(p4)
        % AdaptType(p5))
        .str());
    }
  }
}
#endif //__LOGGING_H_DEFINED__
