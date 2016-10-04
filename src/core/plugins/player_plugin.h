/**
* 
* @file
*
* @brief  PlayerPlugin interface declaration
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "core/src/location.h"
//library includes
#include <analysis/result.h>
#include <core/module_detect.h>
#include <core/plugin.h>

namespace ZXTune
{
  class PlayerPlugin
  {
  public:
    typedef std::shared_ptr<const PlayerPlugin> Ptr;
    typedef ObjectIterator<PlayerPlugin::Ptr> Iterator;
    virtual ~PlayerPlugin() {}

    virtual Plugin::Ptr GetDescription() const = 0; 
    virtual Binary::Format::Ptr GetFormat() const = 0;

    //! @brief Detect modules in data
    virtual Analysis::Result::Ptr Detect(const Parameters::Accessor& params, DataLocation::Ptr inputData, const Module::DetectCallback& callback) const = 0;

    virtual Module::Holder::Ptr Open(const Parameters::Accessor& params, const Binary::Container& data) const = 0;
  };
}
