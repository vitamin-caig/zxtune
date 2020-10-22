/**
* 
* @file
*
* @brief  ArchivePlugin interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "core/src/location.h"
//library includes
#include <analysis/result.h>
#include <core/plugin.h>

namespace Module
{
  class DetectCallback;
}

namespace ZXTune
{
  class ArchivePlugin
  {
  public:
    typedef std::shared_ptr<const ArchivePlugin> Ptr;
    typedef ObjectIterator<ArchivePlugin::Ptr> Iterator;
    virtual ~ArchivePlugin() = default;

    virtual Plugin::Ptr GetDescription() const = 0;
    virtual Binary::Format::Ptr GetFormat() const = 0;

    //! @brief Detect modules in data
    virtual Analysis::Result::Ptr Detect(const Parameters::Accessor& params, DataLocation::Ptr inputData, Module::DetectCallback& callback) const = 0;

    virtual DataLocation::Ptr Open(const Parameters::Accessor& params,
                                   DataLocation::Ptr inputData,
                                   const Analysis::Path& pathToOpen) const = 0; 
  };
}
