/**
 *
 * @file
 *
 * @brief  Multitrack player plugin factory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "core/plugins/archive_plugin.h"
#include "core/plugins/player_plugin.h"
#include "module/players/factory.h"

#include "formats/multitrack.h"

#include "types.h"

#include <memory>

namespace ZXTune
{
  class PluginId;
}  // namespace ZXTune

namespace Module
{
  class MultitrackFactory : public BaseFactory<Formats::Multitrack::Container>
  {
  public:
    using Ptr = std::shared_ptr<const MultitrackFactory>;
  };
}  // namespace Module

namespace ZXTune
{
  PlayerPlugin::Ptr CreatePlayerPlugin(PluginId id, uint_t caps, Formats::Multitrack::Decoder::Ptr decoder,
                                       Module::MultitrackFactory::Ptr factory);
  ArchivePlugin::Ptr CreateArchivePlugin(PluginId id, Formats::Multitrack::Decoder::Ptr decoder,
                                         Module::MultitrackFactory::Ptr factory);
}  // namespace ZXTune
