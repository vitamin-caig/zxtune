/*
Abstract:
  Plugins enumerator implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include "enumerator.h"
#include "../plugin.h"
#include "../error_codes.h"
#include "../module_attrs.h"

#include "players/plugins_list.h"

#include <io/container.h>
#include <io/fs_tools.h>

#include <boost/crc.hpp>
#include <boost/bind.hpp>
#include <boost/ref.hpp>
#include <boost/bind/apply.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/algorithm/string/join.hpp>

#include <text/core.h>

#define FILE_TAG 04EDD719

namespace
{
  using namespace ZXTune;

  typedef std::pair<PluginInformation, CreatePlayerFunc> PlayerPluginDescriptor;
  typedef std::pair<PluginInformation, ImplicitContainerFunc> ImplicitPluginDescription;
  typedef boost::tuple<PluginInformation, OpenNestedFunc, ProcessNestedFunc> NestedPluginDescription;

  inline const OpenNestedFunc& GetOpener(const NestedPluginDescription& npd)
  {
    return npd.get<1>();
  }
  
  const Char ROOT_SUBPATH[] = {'/', 0};

  template<class P>
  inline void DoLog(const DetectParameters::LogFunc& logger, const Char* format, const P& param)
  {
    if (logger)
    {
      logger((Formatter(format) % param).str());
    }
  }

  template<class P1, class P2>
  inline void DoLog(const DetectParameters::LogFunc& logger, const Char* format, const P1& param1, const P2& param2)
  {
    if (logger)
    {
      logger((Formatter(format) % param1 % param2).str());
    }
  }
  
  class PluginsEnumeratorImpl : public PluginsEnumerator
  {
  public:
    PluginsEnumeratorImpl()
    {
      RegisterPlayerPlugins(*this);
    }

    virtual void RegisterPlayerPlugin(const PluginInformation& info, const CreatePlayerFunc& func)
    {
      AllPlugins.push_back(info);
      PlayerPlugins.push_back(PlayerPluginDescriptor(info, func));
    }

    virtual void RegisterImplicitPlugin(const PluginInformation& info, const ImplicitContainerFunc& func)
    {
      AllPlugins.push_back(info);
      ImplicitPlugins.push_back(ImplicitPluginDescription(info, func));
    }
    
    virtual void RegisterNestedPlugin(const PluginInformation& info, 
      const OpenNestedFunc& opener, const ProcessNestedFunc& processor)
    {
      AllPlugins.push_back(info);
      NestedPlugins.push_back(boost::make_tuple(info, opener, processor));
    }

    //public interface
    virtual void EnumeratePlugins(std::vector<PluginInformation>& plugins) const
    {
      plugins = AllPlugins;
    }

    // Open subpath in despite of filter and other
    virtual Error ResolveSubpath(IO::DataContainer::Ptr data, const String& subpath, const DetectParameters::LogFunc& logger,
                              MetaContainer& result) const
    {
      try
      {
        assert(data.get());
        // Navigate through known path
        StringArray containersTmp;
        String openedPath(ROOT_SUBPATH), pathToOpen(subpath);
        IO::DataContainer::Ptr subContainer;
        while (!pathToOpen.empty())
        {
          //check for implicit containers
          {
            IO::DataContainer::Ptr fromImplicit;
            String containerId;
            while (CheckForImplicitContainer((subContainer.get() ? *subContainer : *data), fromImplicit, containerId))
            {
              DoLog(logger, TEXT_MODULE_MESSAGE_OPEN_IMPLICIT, openedPath, containerId);
              subContainer = fromImplicit;
              containersTmp.push_back(containerId);
            }
          }
          //check for other subcontainers
          IO::DataContainer::Ptr fromNestedContainer;
          String restPath;
          String containerId;
          const IO::DataContainer& input(subContainer.get() ? *subContainer : *data);
          if (CheckForNestedContainer(input, pathToOpen, fromNestedContainer, restPath, containerId))
          {
            DoLog(logger, TEXT_MODULE_MESSAGE_OPEN_NESTED, openedPath, containerId);
            subContainer = fromNestedContainer;
            pathToOpen = restPath;
            openedPath = subpath.substr(0, subpath.find(restPath));
            containersTmp.push_back(containerId);
          }
          else
          {
            return MakeFormattedError(THIS_LINE, Module::ERROR_FIND_SUBMODULE, TEXT_MODULE_ERROR_FIND_SUBMODULE, pathToOpen);
          }
        }
        result.Data = subContainer.get() ? subContainer : data;
        result.Path = subpath;
        result.PluginsChain.swap(containersTmp);
        return Error();
      }
      catch (const Error& e)
      {
        return e;
      }
    }

    virtual Error DetectModules(const DetectParameters& params, const MetaContainer& data, ModuleRegion& region) const
    {
      assert(params.Callback);
      //try to detect nested container and pass control there
      {
        const Error& e = DetectNestedContainer(params, data, region);
        if (e != Module::ERROR_FIND_NESTED_MODULE)
        {
          return e;
        }
      }
        
      //try to process implicit
      {
        MetaContainer nested;
        String pluginId;
        const Error& e = DetectImplicitContainer(params.Filter, *data.Data, nested.Data, region, pluginId);
        if (!e)
        {
          DoLog(params.Logger, TEXT_MODULE_MESSAGE_DETECT_IMPLICIT, data.Path, pluginId);
          nested.Path = data.Path;
          nested.PluginsChain = data.PluginsChain;
          nested.PluginsChain.push_back(pluginId);
          return DetectModules(params, nested, region/*invalid affecting*/);
        }
        if (e != Module::ERROR_FIND_IMPLICIT_MODULE)
        {
          return e;
        }
      }
        
      //try to detect and process players
      Module::Player::Ptr player;
      String pluginId;
      const Error& e = DetectPlayer(params.Filter, data, player, region, pluginId);
      if (e)
      {
        //find ok if nothing found -> it's not error
        return e == Module::ERROR_FIND_PLAYER_MODULE ? Error() : e;
      }
      DoLog(params.Logger, TEXT_MODULE_MESSAGE_DETECT_PLAYER, data.Path, pluginId);
      if (const Error& e = params.Callback(player))
      {
        Error err(THIS_LINE, Module::ERROR_DETECT_CANCELED, TEXT_MODULE_ERROR_CANCELED);
        return err.AddSuberror(e);
      }
      return Error();
    }
    
    virtual Error DetectNestedContainer(const DetectParameters& params, const MetaContainer& input,
      ModuleRegion& region) const
    {
      try
      {
        for (std::vector<NestedPluginDescription>::const_iterator it = NestedPlugins.begin(), lim = NestedPlugins.end();
          it != lim; ++it)
        {
          if (params.Filter && params.Filter(it->get<0>()))
          {
            continue;//filtered plugin
          }
          const Error& e = (it->get<2>())(input, params, region);
          //stop on success or canceling
          if (!e || e == Module::ERROR_DETECT_CANCELED)
          {
            return e;
          }
        }
        return Error(THIS_LINE, Module::ERROR_FIND_NESTED_MODULE);//no detailed info (not need)
      }
      catch (const Error& e)
      {
        return e;
      }
    }
    
    virtual Error DetectImplicitContainer(const DetectParameters::FilterFunc& filter, const IO::DataContainer& input,
      IO::DataContainer::Ptr& output, ModuleRegion& region, String& pluginId) const
    {
      try
      {
        for (std::vector<ImplicitPluginDescription>::const_iterator it = ImplicitPlugins.begin(), lim = ImplicitPlugins.end();
          it != lim; ++it)
        {
          if (filter && filter(it->first))
          {
            continue;//filtered plugin
          }
          //find first suitable
          if (it->second(input, output, region))
          {
            pluginId = it->first.Id;
            return Error();
          }
        }
        return Error(THIS_LINE, Module::ERROR_FIND_IMPLICIT_MODULE);//no detailed info (not need)
      }
      catch (const Error& e)
      {
        return e;
      }
    }

    virtual Error DetectPlayer(const DetectParameters::FilterFunc& filter, const MetaContainer& data,
      Module::Player::Ptr& player, ModuleRegion& region, String& pluginId) const
    {
      try
      {
        for (std::vector<PlayerPluginDescriptor>::const_iterator it = PlayerPlugins.begin(), lim = PlayerPlugins.end();
          it != lim; ++it)
        {
          if (filter && filter(it->first))
          {
            continue;//filtered plugin
          }
          //find first suitable
          if (it->second(data, player, region))
          {
            pluginId = it->first.Id;
            return Error();
          }
        }
        return Error(THIS_LINE, Module::ERROR_FIND_PLAYER_MODULE);//no detailed info (not need)
      }
      catch (const Error& e)
      {
        return e;
      }
    }
  private:
    bool CheckForImplicitContainer(const IO::DataContainer& input, IO::DataContainer::Ptr& output, String& containerId) const
    {
      using namespace boost;
      ModuleRegion region;
      const std::vector<ImplicitPluginDescription>::const_iterator it = std::find_if(ImplicitPlugins.begin(), ImplicitPlugins.end(),
        bind(apply<bool>(), bind(&ImplicitPluginDescription::second, _1), cref(input), ref(output), ref(region)));
      if (it != ImplicitPlugins.end())
      {
        containerId = it->first.Id;
        return true;
      }
      return false;
    }

    bool CheckForNestedContainer(const IO::DataContainer& input, const String& pathToOpen,
                                 IO::DataContainer::Ptr& output, String& restPath, String& containerId) const
    {
      using namespace boost;
      const std::vector<NestedPluginDescription>::const_iterator it = std::find_if(NestedPlugins.begin(), NestedPlugins.end(),
        bind(apply<bool>(), bind(GetOpener, _1), cref(input), cref(pathToOpen), 
          ref(output), ref(restPath)));
      if (it != NestedPlugins.end())
      {
        containerId = it->get<0>().Id;
        return true;
      }
      return false;
    }
  private:
    std::vector<PluginInformation> AllPlugins;
    std::vector<NestedPluginDescription> NestedPlugins;
    std::vector<ImplicitPluginDescription> ImplicitPlugins;
    std::vector<PlayerPluginDescriptor> PlayerPlugins;
  };
}

namespace ZXTune
{
  void ExtractMetaProperties(const MetaContainer& container, const ModuleRegion& region, 
                             ParametersMap& properties, Dump& rawData)
  {
    if (!container.Path.empty())
    {
      properties.insert(ParametersMap::value_type(Module::ATTR_SUBPATH, container.Path));
    }
    if (!container.PluginsChain.empty())
    {
      properties.insert(ParametersMap::value_type(Module::ATTR_CONTAINER, 
        boost::algorithm::join(container.PluginsChain, String(TEXT_MODULE_CONTAINERS_DELIMITER))));
    }
    const uint8_t* data(static_cast<const uint8_t*>(container.Data->Data()));
    rawData.assign(data + region.Offset, data + region.Offset + region.Size);
    boost::crc_32_type crcCalc;
    crcCalc.process_bytes(&rawData[0], region.Size);
    properties.insert(ParametersMap::value_type(Module::ATTR_CRC, crcCalc.checksum()));
    properties.insert(ParametersMap::value_type(Module::ATTR_SIZE, region.Size));
  }
  
  PluginsEnumerator& PluginsEnumerator::Instance()
  {
    static PluginsEnumeratorImpl instance;
    return instance;
  }

  void GetSupportedPlugins(std::vector<PluginInformation>& plugins)
  {
    PluginsEnumerator::Instance().EnumeratePlugins(plugins);
  }

  Error DetectModules(IO::DataContainer::Ptr data, const DetectParameters& params, const String& startSubpath)
  {
    if (!data.get() || !params.Callback)
    {
      return Error(THIS_LINE, Module::ERROR_INVALID_PARAMETERS, TEXT_MODULE_ERROR_PARAMETERS);
    }
    const PluginsEnumerator& enumerator(PluginsEnumerator::Instance());
    MetaContainer subcontainer;
    if (const Error& e = enumerator.ResolveSubpath(data, startSubpath, params.Logger, subcontainer))
    {
      return e;
    }
    ModuleRegion region;
    return enumerator.DetectModules(params, subcontainer, region);
  }
}
