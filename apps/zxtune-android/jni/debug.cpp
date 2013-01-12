/*
Abstract:
  Debug functionality implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "debug.h"
//platform includes
#include <android/log.h>

namespace
{
  bool IsLogEnabled(const std::string& module)
  {
    return module.empty();//TODO
  }
}

//stub for Debug library
namespace Debug
{
  void Message(const std::string& module, const std::string& msg)
  {
    __android_log_print(ANDROID_LOG_DEBUG, ("zxtune.so:" + module).c_str(), msg.c_str());
  }

  Stream::Stream(const std::string& module)
    : Module(module)
    , Enabled(IsLogEnabled(module))
  {
  }
}

const Debug::Stream Dbg("");
