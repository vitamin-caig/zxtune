/**
 *
 * @file
 *
 * @brief  Archive plugin implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/archives/packed.h"

#include "core/src/location.h"

#include "analysis/path.h"
#include "analysis/result.h"
#include "binary/format.h"
#include "core/data_location.h"
#include "core/plugin_attrs.h"

#include "make_ptr.h"
#include "string_type.h"
#include "string_view.h"

#include <memory>
#include <span>
#include <utility>

namespace Parameters
{
  class Accessor;
}  // namespace Parameters

namespace ZXTune
{
  const auto ARCHIVE_PLUGIN_PREFIX = "+un"sv;

  String EncodeArchivePluginToPath(PluginId pluginId)
  {
    // TODO: Concat(StringView...)
    return String{ARCHIVE_PLUGIN_PREFIX} + pluginId;
  }

  bool IsArchivePluginPathComponent(StringView component)
  {
    return 0 == component.find(ARCHIVE_PLUGIN_PREFIX);
  }

  StringView DecodeArchivePluginFromPathComponent(StringView component)
  {
    return IsArchivePluginPathComponent(component) ? component.substr(ARCHIVE_PLUGIN_PREFIX.size()) : StringView{};
  }

  class CommonArchivePlugin : public ArchivePlugin
  {
  public:
    CommonArchivePlugin(PluginId id, uint_t caps, Formats::Packed::Decoder::Ptr decoder)
      : Identifier(id)
      , Caps(caps)
      , Decoder(std::move(decoder))
    {}

    PluginId Id() const override
    {
      return Identifier;
    }

    StringView Description() const override
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
        auto subLocation = CreateNestedLocation(std::move(inputData), std::move(subData), Identifier, subPath);
        callback.ProcessData(std::move(subLocation));
        return Analysis::CreateMatchedResult(packedSize);
      }
      auto format = Decoder->GetFormat();
      return Analysis::CreateUnmatchedResult(std::move(format), std::move(rawData));
    }

    DataLocation::Ptr TryOpen(const Parameters::Accessor& /*params*/, DataLocation::Ptr inputData,
                              const Analysis::Path& pathToOpen) const override
    {
      const auto& pathComponent = pathToOpen.Elements().front();
      const auto pluginId = DecodeArchivePluginFromPathComponent(pathComponent);
      if (pluginId != Identifier)
      {
        return {};
      }
      const auto rawData = inputData->GetData();
      if (auto subData = Decoder->Decode(*rawData))
      {
        return CreateNestedLocation(std::move(inputData), std::move(subData), Identifier, pathComponent);
      }
      return {};
    }

  private:
    const PluginId Identifier;
    const uint_t Caps;
    const Formats::Packed::Decoder::Ptr Decoder;
  };
}  // namespace ZXTune

namespace ZXTune
{
  ArchivePlugin::Ptr CreateArchivePlugin(PluginId id, uint_t caps, Formats::Packed::Decoder::Ptr decoder)
  {
    return MakePtr<CommonArchivePlugin>(id, caps | Capabilities::Category::CONTAINER, std::move(decoder));
  }
}  // namespace ZXTune
