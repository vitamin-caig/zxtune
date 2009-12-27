/*
Abstract:
  Plugins enumerator implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include "enumerator.h"
#include "players/plugins_list.h"
#include "implicit/plugins_list.h"
#include "containers/plugins_list.h"

#include <logging.h>

#include <core/error_codes.h>
#include <core/module_attrs.h>
#include <core/plugin.h>
#include <io/container.h>
#include <io/fs_tools.h>

#include <boost/bind.hpp>
#include <boost/crc.hpp>
#include <boost/ref.hpp>
#include <boost/bind/apply.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/algorithm/string/join.hpp>

#include <text/core.h>

#define FILE_TAG 04EDD719

namespace
{
  using namespace ZXTune;

  const std::string THIS_MODULE("Core::Enumerator");
  
  typedef std::pair<PluginInformation, CreateModuleFunc> PlayerPluginDescriptor;
  typedef std::pair<PluginInformation, ProcessImplicitFunc> ImplicitPluginDescription;
  typedef boost::tuple<PluginInformation, OpenContainerFunc, ProcessContainerFunc> ContainerPluginDescription;

  inline const OpenContainerFunc& GetOpener(const ContainerPluginDescription& npd)
  {
    return npd.get<1>();
  }
  
  const Char ROOT_SUBPATH[] = {'/', 0};

  template<class P>
  inline void DoLog(const DetectParameters::LogFunc& logger, const Char* format, const P& param)
  {
    if (logger)
    {
      assert(format);
      logger((Formatter(format) % param).str());
    }
  }

  template<class P1, class P2>
  inline void DoLog(const DetectParameters::LogFunc& logger, const Char* format, const P1& param1, const P2& param2)
  {
    if (logger)
    {
      assert(format);
      logger((Formatter(format) % param1 % param2).str());
    }
  }
  
  class PluginsEnumeratorImpl : public PluginsEnumerator
  {
  public:
    PluginsEnumeratorImpl()
    {
      RegisterContainerPlugins(*this);
      RegisterImplicitPlugins(*this);
      RegisterPlayerPlugins(*this);
    }

    virtual void RegisterPlayerPlugin(const PluginInformation& info, const CreateModuleFunc& func)
    {
      AllPlugins.push_back(info);
      PlayerPlugins.push_back(PlayerPluginDescriptor(info, func));
      Log::Debug(THIS_MODULE, "Registered player %1%", info.Id);
    }

    virtual void RegisterImplicitPlugin(const PluginInformation& info, const ProcessImplicitFunc& func)
    {
      AllPlugins.push_back(info);
      ImplicitPlugins.push_back(ImplicitPluginDescription(info, func));
      Log::Debug(THIS_MODULE, "Registered implicit container %1%", info.Id);
    }
    
    virtual void RegisterContainerPlugin(const PluginInformation& info,
      const OpenContainerFunc& opener, const ProcessContainerFunc& processor)
    {
      AllPlugins.push_back(info);
      ContainerPlugins.push_back(boost::make_tuple(info, opener, processor));
      Log::Debug(THIS_MODULE, "Registered container %1%", info.Id);
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
        String pathToOpen(subpath);
        
        MetaContainer tmpResult;
        tmpResult.Path = ROOT_SUBPATH;
        tmpResult.Data = data;
        while (!pathToOpen.empty())
        {
          Log::Debug(THIS_MODULE, "Resolving subpath '%1%'", pathToOpen);
          //check for implicit containers
          {
            IO::DataContainer::Ptr fromImplicit;
            String containerId;
            if (CheckForImplicit(tmpResult, fromImplicit, containerId))
            {
              Log::Debug(THIS_MODULE, "Detected implicit plugin %1% at '%2%'", containerId, tmpResult.Path);
              DoLog(logger, TEXT_MODULE_MESSAGE_OPEN_IMPLICIT, tmpResult.Path, containerId);
              tmpResult.Data = fromImplicit;
              tmpResult.PluginsChain.push_back(containerId);
            }
          }
          //check for other subcontainers
          IO::DataContainer::Ptr fromContainer;
          String restPath;
          String containerId;
          if (CheckForContainer(tmpResult, pathToOpen, fromContainer, restPath, containerId))
          {
            Log::Debug(THIS_MODULE, "Detected nested container %1% at '%2%'", containerId, tmpResult.Path);
            DoLog(logger, TEXT_MODULE_MESSAGE_OPEN_NESTED, tmpResult.Path, containerId);
            tmpResult.Data = fromContainer;
            pathToOpen = restPath;
            tmpResult.Path = subpath.substr(0, subpath.find(restPath));
            tmpResult.PluginsChain.push_back(containerId);
          }
          else
          {
            return MakeFormattedError(THIS_LINE, Module::ERROR_FIND_SUBMODULE, TEXT_MODULE_ERROR_FIND_SUBMODULE, pathToOpen);
          }
        }
        Log::Debug(THIS_MODULE, "Resolved path '%1%'", subpath);
        result.Data = tmpResult.Data;
        result.Path = subpath;
        result.PluginsChain.swap(tmpResult.PluginsChain);
        return Error();
      }
      catch (const Error& e)
      {
        return e;
      }
    }

    virtual Error DetectModules(const DetectParameters& params, const MetaContainer& data, ModuleRegion& region) const
    {
      Log::Debug(THIS_MODULE, "Detecting modules in data of size %1%, path '%2%'", data.Data->Size(), data.Path);
      assert(params.Callback);
      //try to detect container and pass control there
      {
        const Error& e = DetectContainer(params, data, region);
        if (e != Module::ERROR_FIND_CONTAINER_PLUGIN)
        {
          return e;
        }
      }
       
      //try to process implicit
      {
        MetaContainer nested;
        String pluginId;
        const Error& e = DetectImplicit(params.Filter, data, nested.Data, region, pluginId);
        if (!e)
        {
          DoLog(params.Logger, TEXT_MODULE_MESSAGE_DETECT_IMPLICIT, data.Path, pluginId);
          nested.Path = data.Path;
          nested.PluginsChain = data.PluginsChain;
          nested.PluginsChain.push_back(pluginId);
          ModuleRegion implRegion;
          return DetectModules(params, nested, implRegion);
        }
        if (e != Module::ERROR_FIND_IMPLICIT_PLUGIN)
        {
          return e;
        }
      }
       
      //try to detect and process single modules
      Module::Holder::Ptr holder;
      String pluginId;
      const Error& e = DetectModule(params.Filter, data, holder, region, pluginId);
      if (e)
      {
        //find ok if nothing found -> it's not error
        if (e == Module::ERROR_FIND_PLAYER_PLUGIN)
        {
          region = ModuleRegion();
          return Error();
        }
        else
        {
          return e;
        }
      }
      Log::Debug(THIS_MODULE, "Detected player plugin %1%", pluginId);
      DoLog(params.Logger, TEXT_MODULE_MESSAGE_DETECT_PLAYER, data.Path, pluginId);
      if (const Error& e = params.Callback(holder))
      {
        Error err(THIS_LINE, Module::ERROR_DETECT_CANCELED, TEXT_MODULE_ERROR_CANCELED);
        return err.AddSuberror(e);
      }
      return Error();
    }

  private:
    Error DetectContainer(const DetectParameters& params, const MetaContainer& input,
      ModuleRegion& region) const
    {
      try
      {
        for (std::vector<ContainerPluginDescription>::const_iterator it = ContainerPlugins.begin(), lim = ContainerPlugins.end();
          it != lim; ++it)
        {
          const PluginInformation& plugInfo(it->get<0>());
          if (params.Filter && params.Filter(plugInfo))
          {
            continue;//filtered plugin
          }
          Log::Debug(THIS_MODULE, " Checking container plugin %1% for path '%2%'", plugInfo.Id, input.Path);
          const Error& e = (it->get<2>())(input, params, region);
          //stop on success or canceling
          if (!e)
          {
            Log::Debug(THIS_MODULE, "  Container plugin %1% for path '%2%' detected at region (%3%;%4%)",
              plugInfo.Id, input.Path, region.Offset, region.Size);
            return e;
          }
          if (e == Module::ERROR_DETECT_CANCELED)
          {
            Log::Debug(THIS_MODULE, "  Canceled");
            return e;
          }
        }
        return Error(THIS_LINE, Module::ERROR_FIND_CONTAINER_PLUGIN);//no detailed info (not need)
      }
      catch (const Error& e)
      {
        return e;
      }
    }
    
    Error DetectImplicit(const DetectParameters::FilterFunc& filter, const MetaContainer& input,
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
          Log::Debug(THIS_MODULE, " Checking implicit container %1% at path '%2%'", it->first.Id, input.Path);
          if (it->second(input, output, region))
          {
            Log::Debug(THIS_MODULE, "  Detected at region (%1%;%2%)", region.Offset, region.Size);
            pluginId = it->first.Id;
            return Error();
          }
        }
        return Error(THIS_LINE, Module::ERROR_FIND_IMPLICIT_PLUGIN);//no detailed info (not need)
      }
      catch (const Error& e)
      {
        return e;
      }
    }

    Error DetectModule(const DetectParameters::FilterFunc& filter, const MetaContainer& input,
      Module::Holder::Ptr& holder, ModuleRegion& region, String& pluginId) const
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
          Log::Debug(THIS_MODULE, " Checking module plugin %1% at path '%2%'", it->first.Id, input.Path);
          if (it->second(input, holder, region))
          {
            Log::Debug(THIS_MODULE, "  Detected at region (%1%;%2%)", region.Offset, region.Size);
            pluginId = it->first.Id;
            return Error();
          }
        }
        return Error(THIS_LINE, Module::ERROR_FIND_PLAYER_PLUGIN);//no detailed info (not need)
      }
      catch (const Error& e)
      {
        return e;
      }
    }

    bool CheckForImplicit(const MetaContainer& input, IO::DataContainer::Ptr& output, String& containerId) const
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

    bool CheckForContainer(const MetaContainer& input, const String& pathToOpen,
                                 IO::DataContainer::Ptr& output, String& restPath, String& containerId) const
    {
      using namespace boost;
      const std::vector<ContainerPluginDescription>::const_iterator it = std::find_if(ContainerPlugins.begin(), ContainerPlugins.end(),
        bind(apply<bool>(), bind(GetOpener, _1), cref(input), cref(pathToOpen), ref(output), ref(restPath)));
      if (it != ContainerPlugins.end())
      {
        containerId = it->get<0>().Id;
        return true;
      }
      return false;
    }
  private:
    std::vector<PluginInformation> AllPlugins;
    std::vector<ContainerPluginDescription> ContainerPlugins;
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
