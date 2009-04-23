#ifndef __PLUGIN_ENUMERATOR_H_DEFINED__
#define __PLUGIN_ENUMERATOR_H_DEFINED__

#include <player.h>

namespace ZXTune
{
  typedef bool (*CheckFunc)(const String& filename, const Dump& data);
  typedef ModulePlayer::Ptr (*FactoryFunc)(const String& filename, const Dump& data);
  typedef void (*InfoFunc)(ModulePlayer::Info& info);

  struct PluginDescriptor
  {
    CheckFunc Checker;
    FactoryFunc Creator;
    InfoFunc Descriptor;
  };

  class PluginEnumerator
  {
  public:
    virtual ~PluginEnumerator()
    {
    }

    virtual void RegisterPlugin(const PluginDescriptor& descr) = 0;
    virtual void EnumeratePlugins(std::vector<ModulePlayer::Info>& infos) const = 0;
    virtual ModulePlayer::Ptr CreatePlayer(const String& filename, const Dump& data) const = 0;

    static PluginEnumerator& Instance();
  };

  struct PluginAutoRegistrator
  {
    PluginAutoRegistrator(CheckFunc check, FactoryFunc create, InfoFunc describe)
    {
      PluginDescriptor descr = {check, create, describe};
      PluginEnumerator::Instance().RegisterPlugin(descr);
    }
  };
}

#endif //__PLUGIN_ENUMERATOR_H_DEFINED__
