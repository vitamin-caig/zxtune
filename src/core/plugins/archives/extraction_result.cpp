/*
Abstract:
  Archive extraction result implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "extraction_result.h"
#include "core/src/callback.h"
#include "core/src/core.h"
//boost includes
#include <boost/make_shared.hpp>
//text includes
#include <core/text/plugins.h>

namespace
{
  using namespace ZXTune;

  const String ARCHIVE_PLUGIN_PREFIX(Text::ARCHIVE_PLUGIN_PREFIX);

  String EncodeArchivePluginToPath(const String& pluginId)
  {
    return ARCHIVE_PLUGIN_PREFIX + pluginId;
  }

  bool IsArchivePluginPathComponent(const String& component)
  {
    return 0 == component.find(ARCHIVE_PLUGIN_PREFIX);
  }

  String DecodeArchivePluginFromPathComponent(const String& component)
  {
    assert(IsArchivePluginPathComponent(component));
    return component.substr(ARCHIVE_PLUGIN_PREFIX.size());
  }
}

namespace ZXTune
{
  DetectionResult::Ptr DetectModulesInArchive(Plugin::Ptr plugin, const Formats::Packed::Decoder& decoder, 
    DataLocation::Ptr inputData, const Module::DetectCallback& callback)
  {
    const IO::DataContainer::Ptr rawData = inputData->GetData();
    std::size_t packedSize = 0;
    std::auto_ptr<Dump> res = decoder.Decode(rawData->Data(), rawData->Size(), packedSize);
    if (res.get())
    {
      const ZXTune::Module::NoProgressDetectCallbackAdapter noProgressCallback(callback);
      const IO::DataContainer::Ptr subData = IO::CreateDataContainer(res);
      const String subPath = EncodeArchivePluginToPath(plugin->Id());
      const ZXTune::DataLocation::Ptr subLocation = CreateNestedLocation(inputData, subData, plugin, subPath);
      ZXTune::Module::Detect(subLocation, noProgressCallback);
      return DetectionResult::CreateMatched(packedSize);
    }
    const DataFormat::Ptr format = decoder.GetFormat();
    return DetectionResult::CreateUnmatched(format, rawData);
  }

  DataLocation::Ptr OpenDataFromArchive(Plugin::Ptr plugin, const Formats::Packed::Decoder& decoder,
    DataLocation::Ptr inputData, const DataPath& pathToOpen)
  {
    const String pathComponent = pathToOpen.GetFirstComponent();
    if (!IsArchivePluginPathComponent(pathComponent))
    {
      return DataLocation::Ptr();
    }
    const String pluginId = DecodeArchivePluginFromPathComponent(pathComponent);
    if (pluginId != plugin->Id())
    {
      return DataLocation::Ptr();
    }
    const IO::DataContainer::Ptr rawData = inputData->GetData();
    std::size_t packedSize = 0;
    std::auto_ptr<Dump> res = decoder.Decode(rawData->Data(), rawData->Size(), packedSize);
    if (res.get())
    {
      const IO::DataContainer::Ptr subData = IO::CreateDataContainer(res);
      return CreateNestedLocation(inputData, subData, plugin, pathComponent);
    }
    return DataLocation::Ptr();
  }
}
