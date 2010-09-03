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

  class PluginIteratorImpl : public Plugin::Iterator
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

      static void Deleter(PluginStub*)
      {
      }
    };
  public:
    PluginIteratorImpl(PluginsList::const_iterator from,
                       PluginsList::const_iterator to)
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
        return *Pos;
      }
      assert(!"Plugin iterator is out of range");
      static PluginStub stub;
      return Plugin::Ptr(&stub, &PluginStub::Deleter);
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
    PluginsList::const_iterator Pos;
    const PluginsList::const_iterator Limit;
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
      AllPlugins.push_back(plugin);
      PlayerPlugins.push_back(plugin);
      Log::Debug(THIS_MODULE, "Registered player %1%", plugin->Id());
    }

    virtual void RegisterPlugin(ImplicitPlugin::Ptr plugin)
    {
      AllPlugins.push_back(plugin);
      ImplicitPlugins.push_back(plugin);
      Log::Debug(THIS_MODULE, "Registered implicit container %1%", plugin->Id());
    }

    virtual void RegisterPlugin(ContainerPlugin::Ptr plugin)
    {
      AllPlugins.push_back(plugin);
      ContainerPlugins.push_back(plugin);
      Log::Debug(THIS_MODULE, "Registered container %1%", plugin->Id());
    }

    //public interface
    virtual Plugin::Iterator::Ptr Enumerate() const
    {
      return Plugin::Iterator::Ptr(new PluginIteratorImpl(AllPlugins.begin(), AllPlugins.end()));
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
        hasResolved = false;
        Log::Debug(THIS_MODULE, "Resolving subpath '%1%'", pathToOpen);
        //check for implicit containers
        while (ResolveImplicit(commonParams, tmpResult))
        {
          const Plugin::Ptr lastPlugin = tmpResult.PluginsChain.back();
          Log::Debug(THIS_MODULE, "Detected implicit plugin %1% at '%2%'", lastPlugin->Id(), tmpResult.Path);
          hasResolved = true;
        }

        //check for other subcontainers
        if (!pathToOpen.empty())
        {
          if (ResolveContainer(commonParams, tmpResult, pathToOpen))
          {
            const Plugin::Ptr lastPlugin = tmpResult.PluginsChain.back();
            Log::Debug(THIS_MODULE, "Detected nested container %1% at '%2%'", lastPlugin->Id(), tmpResult.Path);
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
      Log::Debug(THIS_MODULE, "%3%: Detecting modules in data of size %1%, path '%2%'", 
        data.Data->Size(), data.Path, data.PluginsChain.size());
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
        const Error& e = DetectImplicit(commonParams, detectParams, data, region);
        if (e != Module::ERROR_FIND_IMPLICIT_PLUGIN)
        {
          return e;
        }
      }

      //try to detect and process single modules
      return DetectModule(commonParams, detectParams, data, region);
    }

    virtual Error OpenModule(const Parameters::Map& commonParams, const MetaContainer& input, Module::Holder::Ptr& holder) const
    {
      for (PlayerPluginsArray::const_iterator it = PlayerPlugins.begin(), lim = PlayerPlugins.end();
        it != lim; ++it)
      {
        const PlayerPlugin::Ptr plugin = *it;
        if (!plugin->Check(*input.Data))
        {
          continue;//invalid plugin
        }
        ModuleRegion region;
        if (Module::Holder::Ptr module = plugin->CreateModule(commonParams, input, region))
        {
          Log::Debug(THIS_MODULE, "%2%: Opened player plugin %1%", 
            plugin->Id(), input.PluginsChain.size());
          holder = module;
          return Error();
        }
        //TODO: dispatch heavy checks- return false if not enabled
      }
      return Error(THIS_LINE, Module::ERROR_FIND_PLAYER_PLUGIN);//no detailed info (not need)
    }

  private:
    Error DetectContainer(const Parameters::Map& commonParams, const DetectParameters& detectParams, const MetaContainer& input,
      ModuleRegion& region) const
    {
      for (ContainerPluginsArray::const_iterator it = ContainerPlugins.begin(), lim = ContainerPlugins.end();
        it != lim; ++it)
      {
        const ContainerPlugin::Ptr plugin = *it;
        if (detectParams.Filter && detectParams.Filter(*plugin))
        {
          continue;//filtered plugin
        }
        if (!plugin->Check(*input.Data))
        {
          continue;//invalid plugin
        }
        Log::Debug(THIS_MODULE, "%3%:  Checking container plugin %1% for path '%2%'", 
          plugin->Id(), input.Path, input.PluginsChain.size());
        const Error& e = plugin->Process(commonParams, detectParams, input, region);
        //stop on success or canceling
        if (e == Module::ERROR_FIND_CONTAINER_PLUGIN)
        {
          continue;
        }
        if (!e)
        {
          Log::Debug(THIS_MODULE, "%5%:  Container plugin %1% for path '%2%' detected at region (%3%;%4%)",
            plugin->Id(), input.Path, region.Offset, region.Size, input.PluginsChain.size());
        }
        else
        {
          Log::Debug(THIS_MODULE, "%1%:  Canceled", input.PluginsChain.size());
        }
        return e;
      }
      return Error(THIS_LINE, Module::ERROR_FIND_CONTAINER_PLUGIN);//no detailed info (not need)
    }

    Error DetectImplicit(const Parameters::Map& commonParams, const DetectParameters& detectParams, const MetaContainer& input,
      ModuleRegion& region) const
    {
      for (ImplicitPluginsArray::const_iterator it = ImplicitPlugins.begin(), lim = ImplicitPlugins.end();
        it != lim; ++it)
      {
        const ImplicitPlugin::Ptr plugin = *it;
        if (detectParams.Filter && detectParams.Filter(*plugin))
        {
          continue;//filtered plugin
        }
        if (!plugin->Check(*input.Data))
        {
          continue;//invalid plugin
        }
        //find first suitable
        Log::Debug(THIS_MODULE, "%3%:  Checking implicit container %1% at path '%2%'", 
          plugin->Id(), input.Path, input.PluginsChain.size());
        if (IO::DataContainer::Ptr subdata = plugin->ExtractSubdata(commonParams, input, region))
        {
          Log::Debug(THIS_MODULE, "%3%:  Detected at region (%1%;%2%)", 
            region.Offset, region.Size, input.PluginsChain.size());
          const uint_t level = CalculateContainersNesting(input.PluginsChain);
          DoLog(detectParams.Logger, level,
            input.Path.empty() ? Text::MODULE_PROGRESS_DETECT_IMPLICIT_NOPATH : Text::MODULE_PROGRESS_DETECT_IMPLICIT,
            plugin->Id(), input.Path);

          MetaContainer nested;
          nested.Data = subdata;
          nested.Path = input.Path;
          nested.PluginsChain = input.PluginsChain;
          nested.PluginsChain.push_back(plugin);
          ModuleRegion nestedRegion;
          return DetectModules(commonParams, detectParams, nested, nestedRegion);
        }
        //TODO: dispatch heavy checks- return false if not enabled
      }
      return Error(THIS_LINE, Module::ERROR_FIND_IMPLICIT_PLUGIN);//no detailed info (not need)
    }

    Error DetectModule(const Parameters::Map& commonParams, const DetectParameters& detectParams, const MetaContainer& input,
      ModuleRegion& region) const
    {
      for (PlayerPluginsArray::const_iterator it = PlayerPlugins.begin(), lim = PlayerPlugins.end();
        it != lim; ++it)
      {
        const PlayerPlugin::Ptr plugin = *it;
        if (detectParams.Filter && detectParams.Filter(*plugin))
        {
          continue;//filtered plugin
        }
        if (!plugin->Check(*input.Data))
        {
          continue;//invalid plugin
        }
        Log::Debug(THIS_MODULE, "%3%:  Checking module plugin %1% at path '%2%'", 
          plugin->Id(), input.Path, input.PluginsChain.size());
        if (Module::Holder::Ptr module = plugin->CreateModule(commonParams, input, region))
        {
          Log::Debug(THIS_MODULE, "%3%:  Detected at region (%1%;%2%)", 
            region.Offset, region.Size, input.PluginsChain.size());
          const uint_t level = CalculateContainersNesting(input.PluginsChain);
          DoLog(detectParams.Logger, level, input.Path.empty() ? Text::MODULE_PROGRESS_DETECT_PLAYER_NOPATH : Text::MODULE_PROGRESS_DETECT_PLAYER,
            plugin->Id(), input.Path);
          if (const Error& e = detectParams.Callback(input.Path, module))
          {
            Error err(THIS_LINE, Module::ERROR_DETECT_CANCELED, Text::MODULE_ERROR_CANCELED);
            return err.AddSuberror(e);
          }
          return Error();
        }
        //TODO: dispatch heavy checks- return false if not enabled
      }
      region = ModuleRegion();
      return Error();
    }

    bool ResolveImplicit(const Parameters::Map& commonParams, MetaContainer& data) const
    {
      for (ImplicitPluginsArray::const_iterator it = ImplicitPlugins.begin(), lim = ImplicitPlugins.end();
        it != lim; ++it)
      {
        const ImplicitPlugin::Ptr plugin = *it;
        if (!plugin->Check(*data.Data))
        {
          continue;
        }
        ModuleRegion resRegion;
        if (IO::DataContainer::Ptr subdata = plugin->ExtractSubdata(commonParams, data, resRegion))
        {
          data.Data = subdata;
          data.PluginsChain.push_back(plugin);
          return true;
        }
        //TODO: dispatch heavy checks- return false if not enabled
      }
      return false;
    }

    bool ResolveContainer(const Parameters::Map& commonParams, MetaContainer& data, String& pathToOpen) const
    {
      for (ContainerPluginsArray::const_iterator it = ContainerPlugins.begin(), lim = ContainerPlugins.end();
        it != lim; ++it)
      {
        const ContainerPlugin::Ptr plugin = *it;
        if (!plugin->Check(*data.Data))
        {
          continue;
        }
        String restPath;
        if (IO::DataContainer::Ptr subdata = plugin->Open(commonParams, data, pathToOpen, restPath))
        {
          assert(String::npos != pathToOpen.rfind(restPath));
          const String processedPath = pathToOpen.substr(0, pathToOpen.rfind(restPath));
          data.Path += processedPath;
          data.Data = subdata;
          data.PluginsChain.push_back(plugin);
          pathToOpen = restPath;
          return true;
        }  
        //TODO: dispatch heavy checks- return false if not enabled
      }
      return false;
    }
  private:
    PluginsList AllPlugins;
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
      StringArray ids(container.PluginsChain.size());
      std::transform(container.PluginsChain.begin(), container.PluginsChain.end(),
        ids.begin(), boost::mem_fn(&Plugin::Id));
      properties.insert(Parameters::Map::value_type(Module::ATTR_CONTAINER,
        boost::algorithm::join(ids, String(Text::MODULE_CONTAINERS_DELIMITER))));
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

  uint_t CalculateContainersNesting(const PluginsList& pluginsChain)
  {
    uint_t res = 0;
    for (PluginsList::const_iterator it = pluginsChain.begin(), lim = pluginsChain.end(); it != lim; ++it)
    {
      const Plugin& plugin = **it;
      const uint_t caps = plugin.Capabilities();
      if (0 != (caps & CAP_STOR_MULTITRACK))
      {
        ++res;
      }
    }
    return res;
  }


  Plugin::Iterator::Ptr EnumeratePlugins()
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
