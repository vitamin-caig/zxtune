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

    //! @brief Checking if data contains module
    //! @return true if possibly yes, false if defenitely no
    virtual bool Check(const IO::DataContainer& inputData) const = 0;

    virtual DataFormat::Ptr GetFormat() const = 0;

    virtual Module::Holder::Ptr CreateModule(Module::ModuleProperties::RWPtr properties, Parameters::Accessor::Ptr parameters, IO::DataContainer::Ptr data, std::size_t& usedSize) const = 0;
  };

  namespace Module
  {
    class DetectCallback;
  }

  DetectionResult::Ptr DetectModuleInLocation(ModulesFactory::Ptr factory, Plugin::Ptr plugin, DataLocation::Ptr inputData, const Module::DetectCallback& callback);

  PlayerPlugin::Ptr CreatePlayerPlugin(const String& id, const String& info, const String& version, uint_t caps,
    ModulesFactory::Ptr factory);
}

#endif //__CORE_PLUGINS_PLAYER_CREATION_RESULT_H_DEFINED__
