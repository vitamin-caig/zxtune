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
//std includes
#include <algorithm>
#include <cctype>
//boost includes
#include <boost/algorithm/string/trim.hpp>

inline String OptimizeString(const String& str, Char replace = '\?')
{
  // applicable only printable chars in range 0x21..0x7f
  String res(boost::algorithm::trim_copy_if(str, !boost::is_from_range('\x21', '\x7f')));
  std::replace_if(res.begin(), res.end(), std::ptr_fun<int, int>(&std::iscntrl), replace);
  return res;
}

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
  explicit IndexPathComponent(const String& prefix)
    : Prefix(prefix)
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
