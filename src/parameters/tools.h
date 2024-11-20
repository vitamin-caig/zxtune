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

#include "parameters/accessor.h"
#include "parameters/visitor.h"

#include "string_view.h"

namespace Parameters
{
  template<class T>
  void CopyExistingValue(const Accessor& src, Visitor& dst, StringView name)
  {
    T val = T();
    if (Parameters::FindValue(src, name, val))
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
