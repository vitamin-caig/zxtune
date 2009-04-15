#include "plugin_enumerator.h"

#include <cassert>
#include <algorithm>

namespace
{
  using namespace ZXTune;

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

    virtual void RegisterPlugin(const PluginDescriptor& descr)
    {
      assert(Plugins.end() == std::find_if(Plugins.begin(), Plugins.end(), PDFinder(descr)));
      Plugins.push_back(descr);
    }

    virtual void EnumeratePlugins(std::vector<Player::Info>& infos) const
    {
      infos.resize(Plugins.size());
      std::vector<Player::Info>::iterator out(infos.begin());
      for (std::vector<PluginDescriptor>::const_iterator it = Plugins.begin(), lim = Plugins.end(); it != lim; ++it, ++out)
      {
        (*it->Descriptor)(*out);
      }
    }

    virtual Player::Ptr CreatePlayer(const String& filename, const Dump& data) const
    {
      for (std::vector<PluginDescriptor>::const_iterator it = Plugins.begin(), lim = Plugins.end(); it != lim; ++it)
      {
        if ((*it->Checker)(filename, data))
        {
          return (*it->Creator)(filename, data);
        }
      }
      return Player::Ptr(0);
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

  Player::Ptr Player::Create(const String& filename, const Dump& data)
  {
    return PluginEnumerator::Instance().CreatePlayer(filename, data);
  }

  void GetSupportedPlayers(std::vector<Player::Info>& infos)
  {
    return PluginEnumerator::Instance().EnumeratePlugins(infos);
  }
}