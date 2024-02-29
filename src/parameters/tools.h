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

// library includes
#include <parameters/accessor.h>
#include <parameters/visitor.h>

namespace Parameters
{
  template<class T>
  void CopyExistingValue(const Accessor& src, Visitor& dst, StringView name)
  {
    T val = T();
    if (src.FindValue(name, val))
    {
      dst.SetValue(name, val);
    }
  }

  inline void CopyExistingData(const Accessor& src, Visitor& dst, StringView name)
  {
    if (const auto val = src.FindData(name))
    {
      dst.SetValue(name, *val);
    }
  }
}  // namespace Parameters
