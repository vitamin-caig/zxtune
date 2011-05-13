/*
Abstract:
  Data path interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __CORE_PATH_H_DEFINED__
#define __CORE_PATH_H_DEFINED__

//common includes
#include <types.h>
//boost includes
#include <boost/shared_ptr.hpp>

namespace ZXTune
{
  class DataPath
  {
  public:
    typedef boost::shared_ptr<DataPath> Ptr;
    virtual ~DataPath() {}

    virtual String AsString() const = 0;
    virtual String GetFirstComponent() const = 0;
  };

  DataPath::Ptr CreateDataPath(const String& str);
  DataPath::Ptr CreateMergedDataPath(DataPath::Ptr parent, const String& component);
  //rh.AsString() + result->AsString() = lh.AsString()
  DataPath::Ptr SubstractDataPath(const DataPath& lh, const DataPath& rh);
}

#endif //__CORE_SUBPATH_H_DEFINED__
