#include "plugin_enumerator.h"

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

    result_type operator()(const argument_type& arg) const
    {
      return arg.Checker == Val.Checker && arg.Creator == Val.Creator && arg.Descriptor == Val.Descriptor;
    }

  private:
    const argument_type& Val;
  };

  class PluginEnumeratorImpl : public PluginEnumerator
  {
  public:
    PluginEnumeratorImpl()
    {
    }

    virtual void RegisterPlugin(CheckFunc check, FactoryFunc create, InfoFunc describe)
    {
      const PluginDescriptor descr = {check, create, describe};
      assert(Plugins.end() == std::find_if(Plugins.begin(), Plugins.end(), PDFinder(descr)));
      Plugins.push_back(descr);
    }

    virtual void EnumeratePlugins(std::vector<ModulePlayer::Info>& infos) const
    {
      infos.resize(Plugins.size());
      std::vector<ModulePlayer::Info>::iterator out(infos.begin());
      for (std::vector<PluginDescriptor>::const_iterator it = Plugins.begin(), lim = Plugins.end(); it != lim; ++it, ++out)
      {
        (*it->Descriptor)(*out);
      }
    }

    virtual ModulePlayer::Ptr CreatePlayer(const String& filename, const Dump& data) const
    {
      for (std::vector<PluginDescriptor>::const_iterator it = Plugins.begin(), lim = Plugins.end(); it != lim; ++it)
      {
        if ((*it->Checker)(filename, data))
        {
          return (*it->Creator)(filename, data);
        }
      }
      return ModulePlayer::Ptr(0);
    }
  private:
    std::vector<PluginDescriptor> Plugins;
  };
}

namespace ZXTune
{
  PluginEnumerator& PluginEnumerator::Instance()
  {
    static PluginEnumeratorImpl instance;
    return instance;
  }

  ModulePlayer::Ptr ModulePlayer::Create(const String& filename, const Dump& data)
  {
    return PluginEnumerator::Instance().CreatePlayer(filename, data);
  }

  void GetSupportedPlayers(std::vector<ModulePlayer::Info>& infos)
  {
    return PluginEnumerator::Instance().EnumeratePlugins(infos);
  }
}