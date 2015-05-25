/**
* 
* @file
*
* @brief  Archive plugin implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "archive_supp_common.h"
#include "core/src/callback.h"
#include <core/plugin_attrs.h>
//boost includes
#include <boost/make_shared.hpp>
//text includes
#include <core/text/plugins.h>

namespace ZXTune
{
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

  Analysis::Result::Ptr DetectModulesInArchive(Plugin::Ptr plugin, const Formats::Packed::Decoder& decoder, 
    DataLocation::Ptr inputData, const Module::DetectCallback& callback)
  {
    const Binary::Container::Ptr rawData = inputData->GetData();
    if (Formats::Packed::Container::Ptr subData = decoder.Decode(*rawData))
    {
      const Module::CustomProgressDetectCallbackAdapter noProgressCallback(callback);
      const String subPlugin = plugin->Id();
      const String subPath = EncodeArchivePluginToPath(subPlugin);
      const DataLocation::Ptr subLocation = CreateNestedLocation(inputData, subData, subPlugin, subPath);
      Module::Detect(subLocation, noProgressCallback);
      return Analysis::CreateMatchedResult(subData->PackedSize());
    }
    const Binary::Format::Ptr format = decoder.GetFormat();
    return Analysis::CreateUnmatchedResult(format, rawData);
  }

  DataLocation::Ptr OpenDataFromArchive(Plugin::Ptr plugin, const Formats::Packed::Decoder& decoder,
    DataLocation::Ptr inputData, const Analysis::Path& pathToOpen)
  {
    const String pathComponent = pathToOpen.GetIterator()->Get();
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
    if (Formats::Packed::Container::Ptr subData = decoder.Decode(*rawData))
    {
      return CreateNestedLocation(inputData, subData, pluginId, pathComponent);
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

    virtual Binary::Format::Ptr GetFormat() const
    {
      return Decoder->GetFormat();
    }

    virtual Analysis::Result::Ptr Detect(DataLocation::Ptr inputData, const Module::DetectCallback& callback) const
    {
      return DetectModulesInArchive(Description, *Decoder, inputData, callback);
    }

    virtual DataLocation::Ptr Open(const Parameters::Accessor& /*parameters*/,
                                   DataLocation::Ptr inputData,
                                   const Analysis::Path& pathToOpen) const
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
  ArchivePlugin::Ptr CreateArchivePlugin(const String& id, uint_t caps, Formats::Packed::Decoder::Ptr decoder)
  {
    const Plugin::Ptr description = CreatePluginDescription(id, decoder->GetDescription(), caps | Capabilities::Category::CONTAINER);
    return ArchivePlugin::Ptr(new CommonArchivePlugin(description, decoder));
  }
}
