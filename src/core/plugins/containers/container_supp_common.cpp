/*
Abstract:
  Common container plugins support implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "container_supp_common.h"
#include "trdos_process.h"
#include "core/src/callback.h"
#include "core/src/core.h"
//boost includes
#include <boost/make_shared.hpp>
//text includes
#include <core/text/plugins.h>

namespace
{
  using namespace ZXTune;

  class CommonContainerPlugin : public ArchivePlugin
  {
  public:
    CommonContainerPlugin(Plugin::Ptr descr, ContainerFactory::Ptr factory)
      : Description(descr)
      , Factory(factory)
    {
    }

    virtual Plugin::Ptr GetDescription() const
    {
      return Description;
    }

    virtual DetectionResult::Ptr Detect(DataLocation::Ptr input, const Module::DetectCallback& callback) const
    {
      const IO::DataContainer::Ptr rawData = input->GetData();
      if (const TRDos::Catalogue::Ptr files = Factory->CreateContainer(*callback.GetPluginsParameters(), rawData))
      {
        if (files->GetFilesCount())
        {
          ProcessEntries(input, callback, Description, *files);
        }
        return DetectionResult::CreateMatched(files->GetUsedSize());
      }
      return DetectionResult::CreateUnmatched(Factory->GetFormat(), rawData);
    }

    virtual DataLocation::Ptr Open(const Parameters::Accessor& commonParams, DataLocation::Ptr location, const DataPath& inPath) const
    {
      const String& pathComp = inPath.GetFirstComponent();
      if (pathComp.empty())
      {
        return DataLocation::Ptr();
      }
      const IO::DataContainer::Ptr inData = location->GetData();
      if (const TRDos::Catalogue::Ptr files = Factory->CreateContainer(commonParams, inData))
      {
        if (const TRDos::File::Ptr fileToOpen = files->FindFile(pathComp))
        {
          const IO::DataContainer::Ptr subData = fileToOpen->GetData();
          return CreateNestedLocation(location, subData, Description, pathComp); 
        }
      }
      return DataLocation::Ptr();
    }
  private:
    const Plugin::Ptr Description;
    const ContainerFactory::Ptr Factory;
  };
}

namespace ZXTune
{
  ArchivePlugin::Ptr CreateContainerPlugin(const String& id, const String& info, const String& version, uint_t caps,
    ContainerFactory::Ptr factory)
  {
    const Plugin::Ptr description = CreatePluginDescription(id, info, version, caps);
    return ArchivePlugin::Ptr(new CommonContainerPlugin(description, factory));
  }
}
