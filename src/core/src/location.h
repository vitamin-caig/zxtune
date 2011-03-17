/*
Abstract:
  Data location and related interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __CORE_DATA_LOCATION_H_DEFINED__
#define __CORE_DATA_LOCATION_H_DEFINED__

//local includes
#include "core/plugins/plugins_chain.h"
//common includes
#include <parameters.h>
//library includes
#include <io/container.h>

namespace ZXTune
{
  // Describes piece of data and defenitely location inside
  class DataLocation
  {
  public:
    typedef boost::shared_ptr<const DataLocation> Ptr;
    virtual ~DataLocation() {}

    virtual IO::DataContainer::Ptr GetData() const = 0;
    virtual String GetPath() const = 0;
    virtual PluginsChain::ConstPtr GetPlugins() const = 0;
  };

  //! @param coreParams Parameters for plugins processing
  //! @param data Source data to be processed
  //! @param subpath Subpath in source data to be resolved
  //! @return Object if path is valid. No object elsewhere
  DataLocation::Ptr CreateLocation(Parameters::Accessor::Ptr coreParams, IO::DataContainer::Ptr data);
  DataLocation::Ptr OpenLocation(Parameters::Accessor::Ptr coreParams, IO::DataContainer::Ptr data, const String& subpath);

  DataLocation::Ptr CreateNestedLocation(DataLocation::Ptr parent, Plugin::Ptr subPlugin, IO::DataContainer::Ptr subData, const String& subPath);
}

#endif //__CORE_DATA_LOCATION_H_DEFINED__
