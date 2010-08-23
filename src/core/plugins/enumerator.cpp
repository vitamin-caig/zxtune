/*
Abstract:
  Plugins enumerator implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "enumerator.h"
#include "players/plugins_list.h"
#include "implicit/plugins_list.h"
#include "containers/plugins_list.h"
//common includes
#include <error_tools.h>
#include <logging.h>
//library includes
#include <core/error_codes.h>
#include <core/module_attrs.h>
#include <core/plugin_attrs.h>
#include <io/container.h>
#include <io/fs_tools.h>
//boost includes
#include <boost/bind.hpp>
#include <boost/crc.hpp>
#include <boost/ref.hpp>
#include <boost/bind/apply.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/algorithm/string/join.hpp>
//text includes
#include <core/text/core.h>

#define FILE_TAG 04EDD719

namespace
{
  using namespace ZXTune;

  const std::string THIS_MODULE("Core::Enumerator");

  typedef std::map<String, Plugin::Ptr> PluginsMap;
  typedef std::vector<PlayerPlugin::Ptr> PlayerPluginsArray;
  typedef std::vector<ImplicitPlugin::Ptr> ImplicitPluginsArray;
  typedef std::vector<ContainerPlugin::Ptr> ContainerPluginsArray;

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

  class PluginIteratorImpl : public Plugin::IteratorType
  {
    class PluginStub : public Plugin
    {
    public:
      virtual String Id() const
      {
        return String();
      }

      virtual String Description() const
      {
        return String();
      }

      virtual String Version() const
      {
        return String();
      }

      virtual uint_t Capabilities() const
      {
        return 0;
      }
    };
  public:
    PluginIteratorImpl(PluginsMap::const_iterator from,
                       PluginsMap::const_iterator to)
      : Pos(from), Limit(to)
    {
    }

    virtual bool IsValid() const
    {
      return Pos != Limit;
    }

    virtual Plugin::Ptr Get() const
    {
      //since this implementation is passed to external client, make it as safe as possible
      if (Pos != Limit)
      {
        return Pos->second;
      }
      assert(!"Plugin iterator is out of range");
      return Plugin::Ptr(new PluginStub());
    }

    virtual void Next()
    {
      if (Pos != Limit)
      {
        ++Pos;
      }
      else
      {
        assert(!"Plugin iterator is out of range");
      }
    }
  private:
    PluginsMap::const_iterator Pos;
    const PluginsMap::const_iterator Limit;
  };

  class PluginsEnumeratorImpl : public PluginsEnumerator
  {
  public:
    PluginsEnumeratorImpl()
    {
      RegisterContainerPlugins(*this);
      RegisterImplicitPlugins(*this);
      RegisterPlayerPlugins(*this);
    }

    virtual void RegisterPlugin(PlayerPlugin::Ptr plugin)
    {
      AllPlugins.insert(std::make_pair(plugin->Id(), plugin));
      PlayerPlugins.push_back(plugin);
      Log::Debug(THIS_MODULE, "Registered player %1%", plugin->Id());
    }

    virtual void RegisterPlugin(ImplicitPlugin::Ptr plugin)
    {
      AllPlugins.insert(std::make_pair(plugin->Id(), plugin));
      ImplicitPlugins.push_back(plugin);
      Log::Debug(THIS_MODULE, "Registered implicit container %1%", plugin->Id());
    }

    virtual void RegisterPlugin(ContainerPlugin::Ptr plugin)
    {
      AllPlugins.insert(std::make_pair(plugin->Id(), plugin));
      ContainerPlugins.push_back(plugin);
      Log::Debug(THIS_MODULE, "Registered container %1%", plugin->Id());
    }

    virtual const Plugin& GetPluginById(const String& id) const
    {
      const PluginsMap::const_iterator it = AllPlugins.find(id);
      assert(AllPlugins.end() != it || !"Invalid plugin specified");
      return *it->second;
    }

    //public interface
    virtual Plugin::IteratorPtr Enumerate() const
    {
      return Plugin::IteratorPtr(new PluginIteratorImpl(AllPlugins.begin(), AllPlugins.end()));
    }

    // Open subpath in despite of filter and other
    virtual Error ResolveSubpath(const Parameters::Map& commonParams, IO::DataContainer::Ptr data,
      const String& subpath, MetaContainer& result) const
    {
      assert(data.get());
      // Navigate through known path
      String pathToOpen(subpath);

      MetaContainer tmpResult;
      tmpResult.Path = ROOT_SUBPATH;
      tmpResult.Data = data;

      for (bool hasResolved = true; hasResolved;)
      {
        Log::Debug(THIS_MODULE, "Resolving subpath '%1%'", pathToOpen);
        hasResolved = false;
        //check for implicit containers
        {
          String containerId;
          if (IO::DataContainer::Ptr subdata = CheckForImplicit(commonParams, tmpResult, containerId))
          {
            Log::Debug(THIS_MODULE, "Detected implicit plugin %1% at '%2%'", containerId, tmpResult.Path);
            tmpResult.Data = subdata;
            tmpResult.PluginsChain.push_back(containerId);
            hasResolved = true;
          }
        }
        //check for other subcontainers
        if (!pathToOpen.empty())
        {
          IO::DataContainer::Ptr fromContainer;
          String restPath;
          String containerId;
          if (IO::DataContainer::Ptr subdata =
            CheckForContainer(commonParams, tmpResult, pathToOpen, restPath, containerId))
          {
            Log::Debug(THIS_MODULE, "Detected nested container %1% at '%2%'", containerId, tmpResult.Path);
            tmpResult.Data = subdata;
            pathToOpen = restPath;
            assert(String::npos != subpath.rfind(restPath));
            tmpResult.Path = subpath.substr(0, subpath.rfind(restPath));
            tmpResult.PluginsChain.push_back(containerId);
            hasResolved = true;
          }
          else
          {
            return MakeFormattedError(THIS_LINE, Module::ERROR_FIND_SUBMODULE, Text::MODULE_ERROR_FIND_SUBMODULE, pathToOpen);
          }
        }
      }
      Log::Debug(THIS_MODULE, "Resolved path '%1%'", subpath);
      result.Data = tmpResult.Data;
      result.Path = subpath;
      result.PluginsChain.swap(tmpResult.PluginsChain);
      return Error();
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
          const uint_t level = CalculateContainersNesting(data.PluginsChain);
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
      if (const Error& e = DetectModule(commonParams, detectParams.Filter, data, holder, region, pluginId))
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
      const uint_t level = CalculateContainersNesting(data.PluginsChain);
      DoLog(detectParams.Logger, level, data.Path.empty() ? Text::MODULE_PROGRESS_DETECT_PLAYER_NOPATH : Text::MODULE_PROGRESS_DETECT_PLAYER,
        pluginId, data.Path);
      if (const Error& e = detectParams.Callback(data.Path, holder))
      {
        Error err(THIS_LINE, Module::ERROR_DETECT_CANCELED, Text::MODULE_ERROR_CANCELED);
        return err.AddSuberror(e);
      }
      return Error();
    }

    virtual Error OpenModule(const Parameters::Map& commonParams, const MetaContainer& data, Module::Holder::Ptr& holder) const
    {
      ModuleRegion region;
      std::string pluginId;
      if (const Error& e = DetectModule(commonParams, 0, data, holder, region, pluginId))
      {
        return e;
      }
      Log::Debug(THIS_MODULE, "Opened player plugin %1%", pluginId);
      return Error();
    }

  private:
    Error DetectContainer(const Parameters::Map& commonParams, const DetectParameters& detectParams, const MetaContainer& input,
      ModuleRegion& region) const
    {
      for (ContainerPluginsArray::const_iterator it = ContainerPlugins.begin(), lim = ContainerPlugins.end();
        it != lim; ++it)
      {
        const ContainerPlugin& plugin = **it;
        if (detectParams.Filter && detectParams.Filter(plugin))
        {
          continue;//filtered plugin
        }
        const String& id = plugin.Id();
        Log::Debug(THIS_MODULE, " Checking container plugin %1% for path '%2%'", id, input.Path);
        const Error& e = plugin.Process(commonParams, detectParams, input, region);
        //stop on success or canceling
        if (e == Module::ERROR_FIND_CONTAINER_PLUGIN)
        {
          continue;
        }
        if (!e)
        {
          Log::Debug(THIS_MODULE, "  Container plugin %1% for path '%2%' detected at region (%3%;%4%)",
            id, input.Path, region.Offset, region.Size);
        }
        else
        {
          Log::Debug(THIS_MODULE, "  Canceled");
        }
        return e;
      }
      return Error(THIS_LINE, Module::ERROR_FIND_CONTAINER_PLUGIN);//no detailed info (not need)
    }

    Error DetectImplicit(const Parameters::Map& commonParams, const DetectParameters::FilterFunc& filter, const MetaContainer& input,
      IO::DataContainer::Ptr& output, ModuleRegion& region, String& pluginId) const
    {
      for (ImplicitPluginsArray::const_iterator it = ImplicitPlugins.begin(), lim = ImplicitPlugins.end();
        it != lim; ++it)
      {
        const ImplicitPlugin& plugin = **it;
        if (filter && filter(plugin))
        {
          continue;//filtered plugin
        }
        const String& id = plugin.Id();
        //find first suitable
        Log::Debug(THIS_MODULE, " Checking implicit container %1% at path '%2%'", id, input.Path);
        ModuleRegion resRegion;
        if (IO::DataContainer::Ptr subdata =
          plugin.ExtractSubdata(commonParams, input, resRegion))
        {
          Log::Debug(THIS_MODULE, "  Detected at region (%1%;%2%)", resRegion.Offset, resRegion.Size);
          //assign result data
          output = subdata;
          region = resRegion;
          pluginId = id;
          return Error();
        }
      }
      return Error(THIS_LINE, Module::ERROR_FIND_IMPLICIT_PLUGIN);//no detailed info (not need)
    }

    Error DetectModule(const Parameters::Map& commonParams, const DetectParameters::FilterFunc& filter, const MetaContainer& input,
      Module::Holder::Ptr& holder, ModuleRegion& region, String& pluginId) const
    {
      for (PlayerPluginsArray::const_iterator it = PlayerPlugins.begin(), lim = PlayerPlugins.end();
        it != lim; ++it)
      {
        const PlayerPlugin& plugin = **it;
        if (filter && filter(plugin))
        {
          continue;//filtered plugin
        }
        const String& id = plugin.Id();
        //find first suitable
        Log::Debug(THIS_MODULE, " Checking module plugin %1% at path '%2%'", id, input.Path);
        ModuleRegion resRegion;
       if (Module::Holder::Ptr module =
          plugin.CreateModule(commonParams, input, resRegion))
        {
          Log::Debug(THIS_MODULE, "  Detected at region (%1%;%2%)", resRegion.Offset, resRegion.Size);
          //assign result
          holder = module;
          region = resRegion;
          pluginId = id;
          return Error();
        }
      }
      return Error(THIS_LINE, Module::ERROR_FIND_PLAYER_PLUGIN);//no detailed info (not need)
    }

    IO::DataContainer::Ptr CheckForImplicit(const Parameters::Map& commonParams, const MetaContainer& input, String& containerId) const
    {
      for (ImplicitPluginsArray::const_iterator it = ImplicitPlugins.begin(), lim = ImplicitPlugins.end();
        it != lim; ++it)
      {
        const ImplicitPlugin& plugin = **it;
        ModuleRegion resRegion;
        if (IO::DataContainer::Ptr subdata =
          plugin.ExtractSubdata(commonParams, input, resRegion))
        {
          containerId = plugin.Id();
          return subdata;
        }
      }
      return IO::DataContainer::Ptr();
    }

    IO::DataContainer::Ptr CheckForContainer(const Parameters::Map& commonParams, const MetaContainer& input, const String& pathToOpen,
      String& restPath, String& containerId) const
    {
      for (ContainerPluginsArray::const_iterator it = ContainerPlugins.begin(), lim = ContainerPlugins.end();
        it != lim; ++it)
      {
        const ContainerPlugin& plugin = **it;
        String resRestPath;
        if (IO::DataContainer::Ptr subdata =
          plugin.Open(commonParams, input, pathToOpen, resRestPath))
        {
          restPath = resRestPath;
          containerId = plugin.Id();
          return subdata;
        }
      }
      return IO::DataContainer::Ptr();
    }
  private:
    PluginsMap AllPlugins;
    ContainerPluginsArray ContainerPlugins;
    ImplicitPluginsArray ImplicitPlugins;
    PlayerPluginsArray PlayerPlugins;
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

  uint_t CalculateContainersNesting(const StringArray& pluginsChain)
  {
    const PluginsEnumerator& enumerator = PluginsEnumerator::Instance();
    uint_t res = 0;
    for (StringArray::const_iterator it = pluginsChain.begin(), lim = pluginsChain.end(); it != lim; ++it)
    {
      const Plugin& plugin = enumerator.GetPluginById(*it);
      const uint_t caps = plugin.Capabilities();
      if (0 != (caps & CAP_STOR_MULTITRACK))
      {
        ++res;
      }
    }
    return res;
  }


  Plugin::IteratorPtr EnumeratePlugins()
  {
    return PluginsEnumerator::Instance().Enumerate();
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
      if (const Error& e = enumerator.ResolveSubpath(commonParams, data, startSubpath, subcontainer))
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

  Error OpenModule(const Parameters::Map& commonParams, IO::DataContainer::Ptr data, const String& subpath,
      Module::Holder::Ptr& result)
  {
    if (!data.get())
    {
      return Error(THIS_LINE, Module::ERROR_INVALID_PARAMETERS, Text::MODULE_ERROR_PARAMETERS);
    }
    try
    {
      const PluginsEnumerator& enumerator(PluginsEnumerator::Instance());
      MetaContainer subcontainer;
      if (const Error& e = enumerator.ResolveSubpath(commonParams, data, subpath, subcontainer))
      {
        return e;
      }
      //try to detect and process single modules
      if (const Error& e = enumerator.OpenModule(commonParams, subcontainer, result))
      {
        return MakeFormattedError(THIS_LINE, Module::ERROR_FIND_SUBMODULE, Text::MODULE_ERROR_FIND_SUBMODULE, subpath)
          .AddSuberror(e);
      }
      return Error();
    }
    catch (const std::bad_alloc&)
    {
      return Error(THIS_LINE, Module::ERROR_NO_MEMORY, Text::MODULE_ERROR_NO_MEMORY);
    }
  }
}
