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

  class CommonArchivePlugin : public ArchivePlugin
  {
  public:
    CommonArchivePlugin(StringView id, uint_t caps, Formats::Packed::Decoder::Ptr decoder)
      : Identifier(id.to_string())
      , Caps(caps)
      , Decoder(std::move(decoder))
    {}

    String Id() const override
    {
      return Identifier;
    }

    String Description() const override
    {
      return Decoder->GetDescription();
    }

    uint_t Capabilities() const override
    {
      return Caps;
    }

    Binary::Format::Ptr GetFormat() const override
    {
      return Decoder->GetFormat();
    }

    Analysis::Result::Ptr Detect(const Parameters::Accessor&, DataLocation::Ptr inputData,
                                 ArchiveCallback& callback) const override
    {
      auto rawData = inputData->GetData();
      if (auto subData = Decoder->Decode(*rawData))
      {
        const auto packedSize = subData->PackedSize();
        auto subPath = EncodeArchivePluginToPath(Identifier);
        auto subLocation =
            CreateNestedLocation(std::move(inputData), std::move(subData), Identifier, std::move(subPath));
        callback.ProcessData(std::move(subLocation));
        return Analysis::CreateMatchedResult(packedSize);
      }
      auto format = Decoder->GetFormat();
      return Analysis::CreateUnmatchedResult(std::move(format), std::move(rawData));
    }

    DataLocation::Ptr TryOpen(const Parameters::Accessor& /*params*/, DataLocation::Ptr inputData,
                              const Analysis::Path& pathToOpen) const override
    {
      auto pathComponent = pathToOpen.GetIterator()->Get();
      if (!IsArchivePluginPathComponent(pathComponent))
      {
        return {};
      }
      auto pluginId = DecodeArchivePluginFromPathComponent(pathComponent);
      if (pluginId != Identifier)
      {
        return {};
      }
      const auto rawData = inputData->GetData();
      if (auto subData = Decoder->Decode(*rawData))
      {
        return CreateNestedLocation(std::move(inputData), std::move(subData), std::move(pluginId),
                                    std::move(pathComponent));
      }
      return {};
    }

  private:
    const String Identifier;
    const uint_t Caps;
    const Formats::Packed::Decoder::Ptr Decoder;
  };
}  // namespace ZXTune

namespace ZXTune
{
  ArchivePlugin::Ptr CreateArchivePlugin(StringView id, uint_t caps, Formats::Packed::Decoder::Ptr decoder)
  {
    return MakePtr<CommonArchivePlugin>(id, caps | Capabilities::Category::CONTAINER, std::move(decoder));
  }
}  // namespace ZXTune
