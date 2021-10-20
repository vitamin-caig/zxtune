/**
 *
 * @file
 *
 * @brief  Archive plugin implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "core/plugins/archives/packed.h"
#include "core/plugins/plugins_types.h"
#include "core/src/callback.h"
#include <core/plugin_attrs.h>
// common includes
#include <make_ptr.h>
// std includes
#include <utility>

namespace ZXTune
{
  const String ARCHIVE_PLUGIN_PREFIX("+un");

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
                                               DataLocation::Ptr inputData, ArchiveCallback& callback)
  {
    auto rawData = inputData->GetData();
    if (auto subData = decoder.Decode(*rawData))
    {
      const auto packedSize = subData->PackedSize();
      auto subPlugin = plugin->Id();
      auto subPath = EncodeArchivePluginToPath(subPlugin);
      auto subLocation =
          CreateNestedLocation(std::move(inputData), std::move(subData), std::move(subPlugin), std::move(subPath));
      callback.ProcessData(std::move(subLocation));
      return Analysis::CreateMatchedResult(packedSize);
    }
    auto format = decoder.GetFormat();
    return Analysis::CreateUnmatchedResult(std::move(format), std::move(rawData));
  }

  DataLocation::Ptr TryOpenDataFromArchive(Plugin::Ptr plugin, const Formats::Packed::Decoder& decoder,
                                           DataLocation::Ptr inputData, const Analysis::Path& pathToOpen)
  {
    auto pathComponent = pathToOpen.GetIterator()->Get();
    if (!IsArchivePluginPathComponent(pathComponent))
    {
      return {};
    }
    auto pluginId = DecodeArchivePluginFromPathComponent(pathComponent);
    if (pluginId != plugin->Id())
    {
      return {};
    }
    const auto rawData = inputData->GetData();
    if (auto subData = decoder.Decode(*rawData))
    {
      return CreateNestedLocation(std::move(inputData), std::move(subData), std::move(pluginId),
                                  std::move(pathComponent));
    }
    return {};
  }

  class CommonArchivePlugin : public ArchivePlugin
  {
  public:
    CommonArchivePlugin(Plugin::Ptr descr, Formats::Packed::Decoder::Ptr decoder)
      : Description(std::move(descr))
      , Decoder(std::move(decoder))
    {}

    Plugin::Ptr GetDescription() const override
    {
      return Description;
    }

    Binary::Format::Ptr GetFormat() const override
    {
      return Decoder->GetFormat();
    }

    Analysis::Result::Ptr Detect(const Parameters::Accessor& params, DataLocation::Ptr inputData,
                                 ArchiveCallback& callback) const override
    {
      return DetectModulesInArchive(Description, *Decoder, std::move(inputData), callback);
    }

    DataLocation::Ptr TryOpen(const Parameters::Accessor& /*params*/, DataLocation::Ptr inputData,
                              const Analysis::Path& pathToOpen) const override
    {
      return TryOpenDataFromArchive(Description, *Decoder, std::move(inputData), pathToOpen);
    }

  private:
    const Plugin::Ptr Description;
    const Formats::Packed::Decoder::Ptr Decoder;
  };
}  // namespace ZXTune

namespace ZXTune
{
  ArchivePlugin::Ptr CreateArchivePlugin(const String& id, uint_t caps, Formats::Packed::Decoder::Ptr decoder)
  {
    auto description = CreatePluginDescription(id, decoder->GetDescription(), caps | Capabilities::Category::CONTAINER);
    return MakePtr<CommonArchivePlugin>(std::move(description), std::move(decoder));
  }
}  // namespace ZXTune
