/*
Abstract:
  Archive extraction result implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "creation_result.h"
//boost includes
#include <boost/make_shared.hpp>

namespace
{
  using namespace ZXTune;

  class ModuleCreationResultImpl : public ModuleCreationResult
  {
  public:
    ModuleCreationResultImpl(ModulesFactory::Ptr factory, PlayerPlugin::Ptr plugin, Parameters::Accessor::Ptr parameters, DataLocation::Ptr inputData)
      : Factory(factory)
      , Plug(plugin)
      , Parameters(parameters)
      , Location(inputData)
      , UsedSize(0)
    {
    }

    virtual std::size_t GetMatchedDataSize() const
    {
      TryToCreate();
      return UsedSize;
    }

    virtual std::size_t GetLookaheadOffset() const
    {
      const DataFormat::Ptr format = Factory->GetFormat();
      const IO::DataContainer::Ptr rawData = Location->GetData();
      return format->Search(rawData->Data(), rawData->Size());
    }

    virtual Module::Holder::Ptr GetModule() const
    {
      TryToCreate();
      return Holder;
    }
  private:
    void TryToCreate() const
    {
      if (UsedSize)
      {
        return;
      }
      const Module::ModuleProperties::Ptr properties = Module::ModuleProperties::Create(Plug, Location);
      const IO::DataContainer::Ptr data = Location->GetData();
      Holder = Factory->CreateModule(properties, Parameters, data, UsedSize);
    }
  private:
    const ModulesFactory::Ptr Factory;
    const PlayerPlugin::Ptr Plug;
    const Parameters::Accessor::Ptr Parameters;
    const DataLocation::Ptr Location;
    mutable std::size_t UsedSize;
    mutable Module::Holder::Ptr Holder;
  };
}

namespace ZXTune
{
  ModuleCreationResult::Ptr CreateModuleFromLocation(ModulesFactory::Ptr factory, PlayerPlugin::Ptr plugin, Parameters::Accessor::Ptr parameters, DataLocation::Ptr inputData)
  {
    return boost::make_shared<ModuleCreationResultImpl>(factory, plugin, parameters, inputData);
  }
}
