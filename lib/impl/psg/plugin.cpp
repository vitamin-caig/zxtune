#include "../plugin_enumerator.h"

#include <player_attrs.h>

namespace
{
  using namespace ZXTune;

  const String TEXT_PSG_INFO("PSG modules support");
  const String TEXT_PSG_VERSION("0.1");

  void Information(Player::Info& info)
  {
    info.Type = Module::MODULE_TYPE_PSG;
    info.Properties.clear();
    info.Properties[ATTR_DESCRIPTION] = TEXT_PSG_INFO;
    info.Properties[ATTR_VERSION] = TEXT_PSG_VERSION;
  }

  bool Checking(const String& filename, const Dump& data)
  {
    return false;
  }

  Player::Ptr Creating(const String& filename, const Dump& data)
  {
    return Player::Ptr(0);
  }

  PluginAutoRegistrator psgReg(Checking, Creating, Information);
}
