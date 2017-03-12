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
//library includes
#include <parameters/convert.h>

inline Log::ProgressCallback::Ptr CreateProgressCallback(const Module::DetectCallback& callback, uint_t limit)
{
  if (Log::ProgressCallback* cb = callback.GetProgress())
  {
    return Log::CreatePercentProgressCallback(limit, *cb);
  }
  return Log::ProgressCallback::Ptr();
}

class IndexPathComponent
{
public:
  explicit IndexPathComponent(String  prefix)
    : Prefix(std::move(prefix))
  {
  }

  template<class IndexType>
  bool GetIndex(const String& str, IndexType& index) const
  {
    Parameters::IntType res = 0;
    if (0 == str.find(Prefix) && 
        Parameters::ConvertFromString(str.substr(Prefix.size()), res))
    {
      index = static_cast<IndexType>(res);
      return true;
    }
    return false;
  }

  template<class IndexType>
  String Build(IndexType idx) const
  {
    return Prefix + Parameters::ConvertToString(idx);
  }
private:
  const String Prefix;
};
