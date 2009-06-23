#ifndef __PLUGIN_ENUMERATOR_H_DEFINED__
#define __PLUGIN_ENUMERATOR_H_DEFINED__

#include <player.h>

namespace ZXTune
{
  namespace IO
  {
    class DataContainer;
  }

  typedef bool (*ModuleCheckFunc)(const String& filename, const IO::DataContainer& data, uint32_t capFilter);
  typedef ModulePlayer::Ptr (*ModuleFactoryFunc)(const String& filename, const IO::DataContainer& data, uint32_t capFilter);
  typedef void (*ModuleInfoFunc)(ModulePlayer::Info& info);

  class PluginEnumerator
  {
  public:
    virtual ~PluginEnumerator()
    {
    }

    virtual void RegisterPlugin(ModuleCheckFunc check, ModuleFactoryFunc create, ModuleInfoFunc describe) = 0;
    virtual void EnumeratePlugins(std::vector<ModulePlayer::Info>& infos) const = 0;

    virtual bool CheckModule(const String& filename, const IO::DataContainer& data, ModulePlayer::Info& info, uint32_t capFilter) const = 0;
    virtual ModulePlayer::Ptr CreatePlayer(const String& filename, const IO::DataContainer& data, uint32_t capFilter) const = 0;

    static PluginEnumerator& Instance();
  };

  struct PluginAutoRegistrator
  {
    PluginAutoRegistrator(ModuleCheckFunc check, ModuleFactoryFunc create, ModuleInfoFunc describe)
    {
      PluginEnumerator::Instance().RegisterPlugin(check, create, describe);
    }
  };
}

#endif //__PLUGIN_ENUMERATOR_H_DEFINED__
