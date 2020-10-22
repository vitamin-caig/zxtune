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
#include "core/plugins/archives/packed.h"
#include "core/src/callback.h"
#include "core/src/detect.h"
#include "core/plugins/plugins_types.h"
#include <core/plugin_attrs.h>
//common includes
#include <make_ptr.h>
//std includes
#include <utility>
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

  Analysis::Result::Ptr DetectModulesInArchive(const Parameters::Accessor& params, Plugin::Ptr plugin, const Formats::Packed::Decoder& decoder, 
    DataLocation::Ptr inputData, Module::DetectCallback& callback)
  {
    const Binary::Container::Ptr rawData = inputData->GetData();
    if (Formats::Packed::Container::Ptr subData = decoder.Decode(*rawData))
    {
      Module::CustomProgressDetectCallbackAdapter noProgressCallback(callback);
      const String subPlugin = plugin->Id();
      const String subPath = EncodeArchivePluginToPath(subPlugin);
      const DataLocation::Ptr subLocation = CreateNestedLocation(inputData, subData, subPlugin, subPath);
      Module::Detect(params, subLocation, noProgressCallback);
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
      : Description(std::move(descr))
      , Decoder(std::move(decoder))
    {
    }

    Plugin::Ptr GetDescription() const override
    {
      return Description;
    }

    Binary::Format::Ptr GetFormat() const override
    {
      return Decoder->GetFormat();
    }

    Analysis::Result::Ptr Detect(const Parameters::Accessor& params, DataLocation::Ptr inputData, Module::DetectCallback& callback) const override
    {
      return DetectModulesInArchive(params, Description, *Decoder, inputData, callback);
    }

    DataLocation::Ptr Open(const Parameters::Accessor& /*params*/,
                                   DataLocation::Ptr inputData,
                                   const Analysis::Path& pathToOpen) const override
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
    return MakePtr<CommonArchivePlugin>(description, decoder);
  }
}
