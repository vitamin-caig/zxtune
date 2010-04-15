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

#include <error_tools.h>
#include <logging.h>
#include <core/error_codes.h>
#include <core/module_attrs.h>
#include <core/plugin_attrs.h>
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

  typedef std::map<String, uint_t> CapabilitiesCache;
  typedef std::pair<PluginInformation, CreateModuleFunc> PlayerPluginDescription;
  typedef std::vector<PlayerPluginDescription> PlayerPluginsArray;
  typedef std::pair<PluginInformation, ProcessImplicitFunc> ImplicitPluginDescription;
  typedef std::vector<ImplicitPluginDescription> ImplicitPluginsArray;
  typedef boost::tuple<PluginInformation, OpenContainerFunc, ProcessContainerFunc> ContainerPluginDescription;
  typedef std::vector<ContainerPluginDescription> ContainerPluginsArray;

  inline const OpenContainerFunc& GetOpener(const ContainerPluginDescription& npd)
  {
    return npd.get<1>();
  }

  const Char ROOT_SUBPATH[] = {'/', 0};

  template<class P1, class P2>
  inline void DoLog(const DetectParameters::LogFunc& logger, uint_t level, const Char* format, const P1& param1, const P2& param2)
  {
    if (logger)
    {
      assert(format);
      Log::MessageData msg;
      msg.Level = level;
      msg.Text = (SafeFormatter(format) % param1 % param2).str();
      logger(msg);
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
      PluginCaps[info.Id] = info.Capabilities;
      PlayerPlugins.push_back(PlayerPluginDescription(info, func));
      Log::Debug(THIS_MODULE, "Registered player %1%", info.Id);
    }

    virtual void RegisterImplicitPlugin(const PluginInformation& info, const ProcessImplicitFunc& func)
    {
      AllPlugins.push_back(info);
      PluginCaps[info.Id] = info.Capabilities;
      ImplicitPlugins.push_back(ImplicitPluginDescription(info, func));
      Log::Debug(THIS_MODULE, "Registered implicit container %1%", info.Id);
    }

    virtual void RegisterContainerPlugin(const PluginInformation& info,
      const OpenContainerFunc& opener, const ProcessContainerFunc& processor)
    {
      AllPlugins.push_back(info);
      PluginCaps[info.Id] = info.Capabilities;
      ContainerPlugins.push_back(boost::make_tuple(info, opener, processor));
      Log::Debug(THIS_MODULE, "Registered container %1%", info.Id);
    }

    //public interface
    virtual void Enumerate(PluginInformationArray& plugins) const
    {
      plugins = AllPlugins;
    }

    // Open subpath in despite of filter and other
    virtual Error ResolveSubpath(const Parameters::Map& commonParams, IO::DataContainer::Ptr data,
      const String& subpath, const DetectParameters::LogFunc& /*logger*/, MetaContainer& result) const
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
            if (CheckForImplicit(commonParams, tmpResult, fromImplicit, containerId))
            {
              Log::Debug(THIS_MODULE, "Detected implicit plugin %1% at '%2%'", containerId, tmpResult.Path);
              tmpResult.Data = fromImplicit;
              tmpResult.PluginsChain.push_back(containerId);
            }
          }
          //check for other subcontainers
          IO::DataContainer::Ptr fromContainer;
          String restPath;
          String containerId;
          if (CheckForContainer(commonParams, tmpResult, pathToOpen, fromContainer, restPath, containerId))
          {
            Log::Debug(THIS_MODULE, "Detected nested container %1% at '%2%'", containerId, tmpResult.Path);
            tmpResult.Data = fromContainer;
            pathToOpen = restPath;
            tmpResult.Path = subpath.substr(0, subpath.find(restPath));
            tmpResult.PluginsChain.push_back(containerId);
          }
          else
          {
            return MakeFormattedError(THIS_LINE, Module::ERROR_FIND_SUBMODULE, Text::MODULE_ERROR_FIND_SUBMODULE, pathToOpen);
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

    virtual Error DetectModules(const Parameters::Map& commonParams, const DetectParameters& detectParams,
      const MetaContainer& data, ModuleRegion& region) const
    {
      Log::Debug(THIS_MODULE, "Detecting modules in data of size %1%, path '%2%'", data.Data->Size(), data.Path);
      assert(detectParams.Callback);
      //try to detect container and pass control there
      {
        const Error& e = DetectContainer(commonParams, detectParams, data, region);
        if (e != Module::ERROR_FIND_CONTAINER_PLUGIN)
        {
          return e;
        }
      }

      //try to process implicit
      {
        MetaContainer nested;
        String pluginId;
        const Error& e = DetectImplicit(commonParams, detectParams.Filter, data, nested.Data, region, pluginId);
        if (!e)
        {
          const uint_t level = CountPluginsInChain(data.PluginsChain, CAP_STOR_MULTITRACK, CAP_STOR_MULTITRACK);
          DoLog(detectParams.Logger, level,
            data.Path.empty() ? Text::MODULE_PROGRESS_DETECT_IMPLICIT_NOPATH : Text::MODULE_PROGRESS_DETECT_IMPLICIT,
            pluginId, data.Path);
          nested.Path = data.Path;
          nested.PluginsChain = data.PluginsChain;
          nested.PluginsChain.push_back(pluginId);
          ModuleRegion implRegion;
          return DetectModules(commonParams, detectParams, nested, implRegion);
        }
        if (e != Module::ERROR_FIND_IMPLICIT_PLUGIN)
        {
          return e;
        }
      }

      //try to detect and process single modules
      Module::Holder::Ptr holder;
      String pluginId;
      const Error& e = DetectModule(commonParams, detectParams.Filter, data, holder, region, pluginId);
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
      const uint_t level = CountPluginsInChain(data.PluginsChain, CAP_STOR_MULTITRACK, CAP_STOR_MULTITRACK);
      DoLog(detectParams.Logger, level, data.Path.empty() ? Text::MODULE_PROGRESS_DETECT_PLAYER_NOPATH : Text::MODULE_PROGRESS_DETECT_PLAYER,
        pluginId, data.Path);
      if (const Error& e = detectParams.Callback(data.Path, holder))
      {
        Error err(THIS_LINE, Module::ERROR_DETECT_CANCELED, Text::MODULE_ERROR_CANCELED);
        return err.AddSuberror(e);
      }
      return Error();
    }

    virtual uint_t CountPluginsInChain(const StringArray& pluginsChain, uint_t capMask, uint_t capValue) const
    {
      assert(PluginCaps.size() == AllPlugins.size());
      uint_t res = 0;
      for (StringArray::const_iterator it = pluginsChain.begin(), lim = pluginsChain.end(); it != lim; ++it)
      {
        const CapabilitiesCache::const_iterator plugIt = PluginCaps.find(*it);
        assert(plugIt != PluginCaps.end());
        if (plugIt != PluginCaps.end() &&
            (plugIt->second & capMask) == capValue)
        {
          ++res;
        }
      }
      return res;
    }

  private:
    Error DetectContainer(const Parameters::Map& commonParams, const DetectParameters& detectParams, const MetaContainer& input,
      ModuleRegion& region) const
    {
      try
      {
        for (ContainerPluginsArray::const_iterator it = ContainerPlugins.begin(), lim = ContainerPlugins.end();
          it != lim; ++it)
        {
          const PluginInformation& plugInfo(it->get<0>());
          if (detectParams.Filter && detectParams.Filter(plugInfo))
          {
            continue;//filtered plugin
          }
          Log::Debug(THIS_MODULE, " Checking container plugin %1% for path '%2%'", plugInfo.Id, input.Path);
          const Error& e = (it->get<2>())(commonParams, detectParams, input, region);
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

    Error DetectImplicit(const Parameters::Map& commonParams, const DetectParameters::FilterFunc& filter, const MetaContainer& input,
      IO::DataContainer::Ptr& output, ModuleRegion& region, String& pluginId) const
    {
      try
      {
        for (ImplicitPluginsArray::const_iterator it = ImplicitPlugins.begin(), lim = ImplicitPlugins.end();
          it != lim; ++it)
        {
          if (filter && filter(it->first))
          {
            continue;//filtered plugin
          }
          //find first suitable
          Log::Debug(THIS_MODULE, " Checking implicit container %1% at path '%2%'", it->first.Id, input.Path);
          if (it->second(commonParams, input, output, region))
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

    Error DetectModule(const Parameters::Map& commonParams, const DetectParameters::FilterFunc& filter, const MetaContainer& input,
      Module::Holder::Ptr& holder, ModuleRegion& region, String& pluginId) const
    {
      try
      {
        for (PlayerPluginsArray::const_iterator it = PlayerPlugins.begin(), lim = PlayerPlugins.end();
          it != lim; ++it)
        {
          if (filter && filter(it->first))
          {
            continue;//filtered plugin
          }
          //find first suitable
          Log::Debug(THIS_MODULE, " Checking module plugin %1% at path '%2%'", it->first.Id, input.Path);
          if (it->second(commonParams, input, holder, region))
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

    bool CheckForImplicit(const Parameters::Map& commonParams, const MetaContainer& input, IO::DataContainer::Ptr& output, String& containerId) const
    {
      using namespace boost;
      ModuleRegion region;
      const ImplicitPluginsArray::const_iterator it = std::find_if(ImplicitPlugins.begin(), ImplicitPlugins.end(),
        bind(apply<bool>(), bind(&ImplicitPluginDescription::second, _1), cref(commonParams), cref(input), ref(output), ref(region)));
      if (it != ImplicitPlugins.end())
      {
        containerId = it->first.Id;
        return true;
      }
      return false;
    }

    bool CheckForContainer(const Parameters::Map& commonParams, const MetaContainer& input, const String& pathToOpen,
      IO::DataContainer::Ptr& output, String& restPath, String& containerId) const
    {
      using namespace boost;
      const ContainerPluginsArray::const_iterator it = std::find_if(ContainerPlugins.begin(), ContainerPlugins.end(),
        bind(apply<bool>(), bind(GetOpener, _1), cref(commonParams), cref(input), cref(pathToOpen), ref(output), ref(restPath)));
      if (it != ContainerPlugins.end())
      {
        containerId = it->get<0>().Id;
        return true;
      }
      return false;
    }
  private:
    PluginInformationArray AllPlugins;
    ContainerPluginsArray ContainerPlugins;
    ImplicitPluginsArray ImplicitPlugins;
    PlayerPluginsArray PlayerPlugins;
    CapabilitiesCache PluginCaps;
  };
}

namespace ZXTune
{
  void ExtractMetaProperties(const String& type,
                             const MetaContainer& container, const ModuleRegion& region, const ModuleRegion& fixedRegion,
                             Parameters::Map& properties, Dump& rawData)
  {
    properties.insert(Parameters::Map::value_type(Module::ATTR_TYPE, type));
    if (!container.Path.empty())
    {
      properties.insert(Parameters::Map::value_type(Module::ATTR_SUBPATH, container.Path));
    }
    if (!container.PluginsChain.empty())
    {
      properties.insert(Parameters::Map::value_type(Module::ATTR_CONTAINER,
        boost::algorithm::join(container.PluginsChain, String(Text::MODULE_CONTAINERS_DELIMITER))));
    }
    const uint8_t* const data = static_cast<const uint8_t*>(container.Data->Data());
    rawData.assign(data + region.Offset, data + region.Offset + region.Size);
    //calculate total checksum
    {
      boost::crc_32_type crcCalc;
      crcCalc.process_bytes(&rawData[0], region.Size);
      properties.insert(Parameters::Map::value_type(Module::ATTR_CRC, crcCalc.checksum()));
    }
    //calculate fixed checksum
    if (fixedRegion.Offset != 0 || fixedRegion.Size != region.Size)
    {
      assert(fixedRegion.Offset + fixedRegion.Size <= region.Size);
      boost::crc_32_type crcCalc;
      crcCalc.process_bytes(&rawData[fixedRegion.Offset], fixedRegion.Size);
      properties.insert(Parameters::Map::value_type(Module::ATTR_FIXEDCRC, crcCalc.checksum()));
    }
    properties.insert(Parameters::Map::value_type(Module::ATTR_SIZE, region.Size));
  }

  PluginsEnumerator& PluginsEnumerator::Instance()
  {
    static PluginsEnumeratorImpl instance;
    return instance;
  }

  void EnumeratePlugins(PluginInformationArray& plugins)
  {
    PluginsEnumerator::Instance().Enumerate(plugins);
  }

  Error DetectModules(const Parameters::Map& commonParams, const DetectParameters& detectParams,
    IO::DataContainer::Ptr data, const String& startSubpath)
  {
    if (!data.get() || !detectParams.Callback)
    {
      return Error(THIS_LINE, Module::ERROR_INVALID_PARAMETERS, Text::MODULE_ERROR_PARAMETERS);
    }
    try
    {
      const PluginsEnumerator& enumerator(PluginsEnumerator::Instance());
      MetaContainer subcontainer;
      if (const Error& e = enumerator.ResolveSubpath(commonParams, data, startSubpath, detectParams.Logger, subcontainer))
      {
        return e;
      }
      ModuleRegion region;
      return enumerator.DetectModules(commonParams, detectParams, subcontainer, region);
    }
    catch (const std::bad_alloc&)
    {
      return Error(THIS_LINE, Module::ERROR_NO_MEMORY, Text::MODULE_ERROR_NO_MEMORY);
    }
  }
}
