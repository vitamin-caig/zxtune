#ifndef __PLUGIN_ENUMERATOR_H_DEFINED__
#define __PLUGIN_ENUMERATOR_H_DEFINED__

#include <player.h>

namespace ZXTune
{
  namespace IO
  {
    class DataContainer;
  }

  typedef bool (*CheckFunc)(const String& filename, const IO::DataContainer& data);
  typedef ModulePlayer::Ptr (*FactoryFunc)(const String& filename, const IO::DataContainer& data);
  typedef void (*InfoFunc)(ModulePlayer::Info& info);

  class PluginEnumerator
  {
  public:
    virtual ~PluginEnumerator()
    {
    }

    virtual void RegisterPlugin(CheckFunc check, FactoryFunc create, InfoFunc describe) = 0;
    virtual void EnumeratePlugins(std::vector<ModulePlayer::Info>& infos) const = 0;

    virtual bool CheckModule(const String& filename, const IO::DataContainer& data, ModulePlayer::Info& info) const = 0;
    virtual ModulePlayer::Ptr CreatePlayer(const String& filename, const IO::DataContainer& data) const = 0;

    static PluginEnumerator& Instance();
  };

  struct PluginAutoRegistrator
  {
    PluginAutoRegistrator(CheckFunc check, FactoryFunc create, InfoFunc describe)
    {
      PluginEnumerator::Instance().RegisterPlugin(check, create, describe);
    }
  };
}

#endif //__PLUGIN_ENUMERATOR_H_DEFINED__
