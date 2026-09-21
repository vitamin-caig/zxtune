/**
 *
 * @file
 *
 * @brief  Delegate factory implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/players/multi/plugins_factory.h"

#include "core/plugins/player_plugin.h"

#include "make_ptr.h"

namespace ZXTune
{
  class PluginsDelegateFactory : public Module::Factory
  {
  public:
    Module::Holder::Ptr CreateModule(const Parameters::Accessor& params, const Binary::Container& data,
                                     Parameters::Container::Ptr properties) const override
    {
      for (const auto& plugin : PlayerPlugin::Enumerate())
      {
        if (auto result = plugin->TryOpen(params, data, properties))
        {
          return result;
        }
      }
      return {};
    }
  };

  Module::Factory::Ptr CreatePluginsDelegateFactory()
  {
    return MakePtr<PluginsDelegateFactory>();
  }
}  // namespace ZXTune
