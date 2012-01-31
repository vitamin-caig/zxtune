/*
Abstract:
  Module container and related interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __CORE_PLUGINS_CORE_H_DEFINED__
#define __CORE_PLUGINS_CORE_H_DEFINED__

//local includes
#include "location.h"
#include "core/plugins/enumerator.h"
//common includes
#include <parameters.h>
//library includes
#include <core/module_holder.h>

namespace Log
{
  class ProgressCallback;
}

namespace ZXTune
{
  namespace Module
  {
    //! @param location Source data location
    //! @return Object is exists. No object elsewhere
    Holder::Ptr Open(DataLocation::Ptr location);

    //! @param location Start data location
    //! @param params Detect callback
    //! @return Size in bytes of source data processed
    std::size_t Detect(DataLocation::Ptr location, const DetectCallback& callback);
  }
}

#endif //__CORE_PLUGINS_CORE_H_DEFINED__
