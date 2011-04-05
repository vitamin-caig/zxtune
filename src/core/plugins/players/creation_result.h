/*
Abstract:
  Module creation result interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __CORE_PLUGINS_PLAYER_CREATION_RESULT_H_DEFINED__
#define __CORE_PLUGINS_PLAYER_CREATION_RESULT_H_DEFINED__

//local includes
#include "module_properties.h"
#include "core/plugins/plugins_types.h"
//library includes
#include <io/container.h>

namespace ZXTune
{
  class ModulesFactory
  {
  public:
    typedef boost::shared_ptr<const ModulesFactory> Ptr;
    virtual ~ModulesFactory() {}

    virtual DataFormat::Ptr GetFormat() const = 0;
    virtual Module::Holder::Ptr CreateModule(Module::ModuleProperties::Ptr properties, Parameters::Accessor::Ptr parameters, IO::DataContainer::Ptr data, std::size_t& usedSize) const = 0;
  };

  namespace Module
  {
    class DetectCallback;
  }

  DetectionResult::Ptr DetectModuleInLocation(ModulesFactory::Ptr factory, PlayerPlugin::Ptr plugin, DataLocation::Ptr inputData, const Module::DetectCallback& callback);

  ModuleCreationResult::Ptr CreateModuleFromLocation(ModulesFactory::Ptr factory, PlayerPlugin::Ptr plugin, Parameters::Accessor::Ptr parameters, DataLocation::Ptr inputData);
}

#endif //__CORE_PLUGINS_PLAYER_CREATION_RESULT_H_DEFINED__
