/**
* 
* @file
*
* @brief  Different utils
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "core/src/callback.h"

inline Log::ProgressCallback::Ptr CreateProgressCallback(const Module::DetectCallback& callback, uint_t limit)
{
  if (Log::ProgressCallback* cb = callback.GetProgress())
  {
    return Log::CreatePercentProgressCallback(limit, *cb);
  }
  return Log::ProgressCallback::Ptr();
}
