#include "plugin_enumerator.h"

#include <player_attrs.h>

#include <cassert>
#include <algorithm>

namespace
{
  using namespace ZXTune;

  struct PluginDescriptor
  {
    CheckFunc Checker;
    FactoryFunc Creator;
    InfoFunc Descriptor;
  };

  class PDFinder : public std::unary_function<PluginDescriptor, bool>
  {
  public:
    PDFinder(const argument_type& val) : Val(val)
    {
    }

    PDFinder(const PDFinder& rh) : Val(rh.Val)
    {
    }

    result_type operator()(const argument_type& arg) const
    {
      return arg.Checker == Val.Checker && arg.Creator == Val.Creator && arg.Descriptor == Val.Descriptor;
    }

  private:
    const argument_type& Val;
  };

  class PluginEnumeratorImpl : public PluginEnumerator
  {
    typedef std::list<PluginDescriptor> PluginsStorage;
  public:
    PluginEnumeratorImpl()
    {
    }

    virtual void RegisterPlugin(CheckFunc check, FactoryFunc create, InfoFunc describe)
    {
      const PluginDescriptor descr = {check, create, describe};
      assert(Plugins.end() == std::find_if(Plugins.begin(), Plugins.end(), PDFinder(descr)));
      ModulePlayer::Info info;
      describe(info);
      //store multitrack plugins to beginning
      if (info.Capabilities & CAP_MULTITRACK)
      {
        Plugins.push_front(descr);
      }
      else
      {
        Plugins.push_back(descr);
      }
    }

    virtual void EnumeratePlugins(std::vector<ModulePlayer::Info>& infos) const
    {
      infos.resize(Plugins.size());
      std::vector<ModulePlayer::Info>::iterator out(infos.begin());
      for (PluginsStorage::const_iterator it = Plugins.begin(), lim = Plugins.end(); it != lim; ++it, ++out)
      {
        (*it->Descriptor)(*out);
      }
    }

    virtual ModulePlayer::Ptr CreatePlayer(const String& filename, const IO::DataContainer& data) const
    {
      for (PluginsStorage::const_iterator it = Plugins.begin(), lim = Plugins.end(); it != lim; ++it)
      {
        if ((*it->Checker)(filename, data))
        {
          return (*it->Creator)(filename, data);
        }
      }
      return ModulePlayer::Ptr(0);
    }
  private:
    PluginsStorage Plugins;
  };
}

namespace ZXTune
{
  PluginEnumerator& PluginEnumerator::Instance()
  {
    static PluginEnumeratorImpl instance;
    return instance;
  }

  ModulePlayer::Ptr ModulePlayer::Create(const String& filename, const IO::DataContainer& data)
  {
    return PluginEnumerator::Instance().CreatePlayer(filename, data);
  }

  void GetSupportedPlayers(std::vector<ModulePlayer::Info>& infos)
  {
    return PluginEnumerator::Instance().EnumeratePlugins(infos);
  }
}