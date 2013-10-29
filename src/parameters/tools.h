/**
*
* @file
*
* @brief  Parameters-related tools
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <parameters/accessor.h>
#include <parameters/visitor.h>

namespace Parameters
{
  template<class T>
  void CopyExistingValue(const Accessor& src, Visitor& dst, const NameType& name)
  {
    T val = T();
    if (src.FindValue(name, val))
    {
      dst.SetValue(name, val);
    }
  }
}
