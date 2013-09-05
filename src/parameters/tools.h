/**
*
* @file     parameters/tools.h
* @brief    Parameters-related tools
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef PARAMETERS_TOOLS_H_DEFINED
#define PARAMETERS_TOOLS_H_DEFINED

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

#endif //PARAMETERS_TOOLS_H_DEFINED
