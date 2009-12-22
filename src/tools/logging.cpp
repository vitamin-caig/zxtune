/*
Abstract:
  Logging functions implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include <logging.h>

#include <iostream>

namespace Log
{
  bool IsDebuggingEnabled()
  {
#ifdef NDEBUG
    return false;//TODO: check environment variables
#else
    return true;
#endif
  }

  void Message(const std::string& module, const std::string& msg)
  {
    std::cerr << '[' << module << "]: " << msg << std::endl;
  }
}
