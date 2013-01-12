/**
*
* @file     debug_log.h
* @brief    Debug logging functions interface
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef DEBUG_LOG_H_DEFINED
#define DEBUG_LOG_H_DEFINED

//common includes
#include <types.h>
//std includes
#include <cassert>
#include <string>
//boost includes
#include <boost/format.hpp>

//! @brief Namespace is used for debug purposes
namespace Debug
{
  //! @brief Unconditionally outputs debug message
  void Message(const std::string& module, const std::string& msg);

  /*
     @brief Per-module debug stream
     @code
       const Debug::Stream Dbg(THIS_MODULE);
       ...
       Dbg("message %1%", parameter)
     @endcode
  */
  class Stream
  {
  public:
    explicit Stream(const std::string& module);

    //! @brief Conditionally outputs debug message from specified module
    void operator ()(const char* msg) const
    {
      assert(msg);
      if (Enabled)
      {
        Message(Module, msg);
      }
    }

    //! @brief Conditionally outputs formatted (up to 5 parameters) debug message from specified module
    //! @note Template functions and public level checking due to performance issues
    template<class P1>
    void operator ()(const char* msg, const P1& p1) const
    {
      assert(msg);
      if (Enabled)
      {
        Message(Module, (boost::format(msg)
          % AdaptType(p1))
          .str());
      }
    }

    template<class P1, class P2>
    void operator ()(const char* msg, const P1& p1, const P2& p2) const
    {
      assert(msg);
      if (Enabled)
      {
        Message(Module, (boost::format(msg)
          % AdaptType(p1)
          % AdaptType(p2))
          .str());
      }
    }

    template<class P1, class P2, class P3>
    void operator ()(const char* msg, const P1& p1, const P2& p2, const P3& p3) const
    {
      assert(msg);
      if (Enabled)
      {
        Message(Module, (boost::format(msg)
          % AdaptType(p1)
          % AdaptType(p2)
          % AdaptType(p3))
          .str());
      }
    }

    template<class P1, class P2, class P3, class P4>
    void operator ()(const char* msg, const P1& p1, const P2& p2, const P3& p3, const P4& p4) const
    {
      assert(msg);
      if (Enabled)
      {
        Message(Module, (boost::format(msg)
          % AdaptType(p1)
          % AdaptType(p2)
          % AdaptType(p3)
          % AdaptType(p4))
          .str());
      }
    }

    template<class P1, class P2, class P3, class P4, class P5>
    void operator ()(const char* msg, const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5) const
    {
      assert(msg);
      if (Enabled)
      {
        Message(Module, (boost::format(msg)
          % AdaptType(p1)
          % AdaptType(p2)
          % AdaptType(p3)
          % AdaptType(p4)
          % AdaptType(p5))
          .str());
      }
    }
  private:
     // Type adapters template for std::string output
     template<class T>
     static inline const T& AdaptType(const T& param)
     {
       return param;
     }

     #ifdef UNICODE
     // Unicode adapter
     static inline std::string AdaptType(const String& param)
     {
       return ToStdString(param);
     }
     #endif
  private:
    const std::string Module;
    const bool Enabled;
  };
}

#endif //DEBUG_LOG_H_DEFINED
