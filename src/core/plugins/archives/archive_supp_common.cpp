/*
Abstract:
  Archive extraction result implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "archive_supp_common.h"
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

  DetectionResult::Ptr DetectModulesInArchive(Plugin::Ptr plugin, const Formats::Packed::Decoder& decoder, 
    DataLocation::Ptr inputData, const Module::DetectCallback& callback)
  {
    const Binary::Container::Ptr rawData = inputData->GetData();
    std::size_t packedSize = 0;
    std::auto_ptr<Dump> res = decoder.Decode(rawData->Data(), rawData->Size(), packedSize);
    if (res.get() && !res->empty())
    {
      const ZXTune::Module::CustomProgressDetectCallbackAdapter noProgressCallback(callback);
      const Binary::Container::Ptr subData = Binary::CreateContainer(res);
      const String subPath = EncodeArchivePluginToPath(plugin->Id());
      const ZXTune::DataLocation::Ptr subLocation = CreateNestedLocation(inputData, subData, plugin, subPath);
      ZXTune::Module::Detect(subLocation, noProgressCallback);
      return DetectionResult::CreateMatched(packedSize);
    }
    const Binary::Format::Ptr format = decoder.GetFormat();
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
    const Binary::Container::Ptr rawData = inputData->GetData();
    std::size_t packedSize = 0;
    std::auto_ptr<Dump> res = decoder.Decode(rawData->Data(), rawData->Size(), packedSize);
    if (res.get())
    {
      const Binary::Container::Ptr subData = Binary::CreateContainer(res);
      return CreateNestedLocation(inputData, subData, plugin, pathComponent);
    }
    return DataLocation::Ptr();
  }

  class CommonArchivePlugin : public ArchivePlugin
  {
  public:
    CommonArchivePlugin(Plugin::Ptr descr, Formats::Packed::Decoder::Ptr decoder)
      : Description(descr)
      , Decoder(decoder)
    {
    }

    virtual Plugin::Ptr GetDescription() const
    {
      return Description;
    }

    virtual DetectionResult::Ptr Detect(DataLocation::Ptr inputData, const Module::DetectCallback& callback) const
    {
      return DetectModulesInArchive(Description, *Decoder, inputData, callback);
    }

    virtual DataLocation::Ptr Open(const Parameters::Accessor& /*parameters*/,
                                   DataLocation::Ptr inputData,
                                   const DataPath& pathToOpen) const
    {
      return OpenDataFromArchive(Description, *Decoder, inputData, pathToOpen);
    }
  private:
    const Plugin::Ptr Description;
    const Formats::Packed::Decoder::Ptr Decoder;
  };
}

namespace ZXTune
{
  ArchivePlugin::Ptr CreateArchivePlugin(const String& id, const String& info, const String& version, uint_t caps,
    Formats::Packed::Decoder::Ptr decoder)
  {
    const Plugin::Ptr description = CreatePluginDescription(id, info, version, caps);
    return ArchivePlugin::Ptr(new CommonArchivePlugin(description, decoder));
  }
}
