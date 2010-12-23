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
#include <tools.h>
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
#include <boost/make_shared.hpp>
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

  const Char IMPLICIT_PLUGIN_PREFIX[] = {'#', 'u', 'n', 0};

  String EncodeImplicitPluginToPath(const String& pluginId)
  {
    return String(IMPLICIT_PLUGIN_PREFIX) + pluginId;
  }

  String DecodeImplicitPluginFromPath(const String& path)
  {
    return 0 == path.find(IMPLICIT_PLUGIN_PREFIX)
      ? path.substr(ArraySize(IMPLICIT_PLUGIN_PREFIX) - 1)
      : String();
  }

  class LoggerHelper
  {
  public:
    LoggerHelper(const DetectParameters& detectParams, const MetaContainer& input, const String& msgFormat)
      : Params(detectParams)
      , Path(input.Path)
      , Format(msgFormat)
    {
      const uint_t level = input.Plugins->CalculateContainersNesting();
      Message.Level = level;
    }

    void operator () (const Plugin& plugin)
    {
      Message.Text = (SafeFormatter(Format) % plugin.Id() % Path).str();
      Params.ReportMessage(Message);
    }
  private:
    const DetectParameters& Params;
    const String Path;
    const String Format;
    Log::MessageData Message;
  };

  typedef std::list<Plugin::Ptr> PluginsList;

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

  class PluginIteratorImpl : public Plugin::Iterator
  {
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

  class PluginsChainImpl : public PluginsChain
  {
  public:
    PluginsChainImpl()
      : NestingCache(-1)
    {
    }

    PluginsChainImpl(const PluginsList& list)
      : Container(list)
      , NestingCache(-1)
    {
    }

    virtual void Add(Plugin::Ptr plugin)
    {
      Container.push_back(plugin);
      NestingCache = -1;
    }

    virtual Plugin::Ptr GetLast() const
    {
      if (!Container.empty())
      {
        return Container.back();
      }
      assert(!"No plugins in chain");
      static PluginStub stub;
      return Plugin::Ptr(&stub, &PluginStub::Deleter);
    }

    virtual PluginsChain::Ptr Clone() const
    {
      return boost::make_shared<PluginsChainImpl>(Container);
    }

    virtual uint_t Count() const
    {
      return Container.size();
    }

    virtual String AsString() const
    {
      StringArray ids(Container.size());
      std::transform(Container.begin(), Container.end(),
        ids.begin(), boost::mem_fn(&Plugin::Id));
      return boost::algorithm::join(ids, String(Text::MODULE_CONTAINERS_DELIMITER));
    }

    virtual uint_t CalculateContainersNesting() const
    {
      if (-1 == NestingCache)
      {
        NestingCache = std::count_if(Container.begin(), Container.end(),
          &IsMultitrackPlugin);
      }
      return static_cast<uint_t>(NestingCache);
    }
  private:
    static bool IsMultitrackPlugin(const Plugin::Ptr plugin)
    {
      const uint_t caps = plugin->Capabilities();
      return CAP_STOR_MULTITRACK == (caps & CAP_STOR_MULTITRACK);
    }
  private:
    PluginsList Container;
    mutable int_t NestingCache;
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
    virtual void ResolveSubpath(const Parameters::Accessor& commonParams, IO::DataContainer::Ptr data,
      const String& subpath, MetaContainer& result) const
    {
      assert(data.get());
      // Navigate through known path
      String pathToOpen(subpath);

      MetaContainer tmpResult;
      tmpResult.Path = ROOT_SUBPATH;
      tmpResult.Data = data;

      while (!pathToOpen.empty())
      {
        Log::Debug(THIS_MODULE, "Resolving '%1%' + '%2%'", tmpResult.Path, pathToOpen);
        //check for implicit containers
        if (ResolveImplicit(commonParams, tmpResult, pathToOpen))
        {
          continue;
        }

        //check for other subcontainers
        if (ResolveContainer(commonParams, tmpResult, pathToOpen))
        {
          continue;
        }
        throw MakeFormattedError(THIS_LINE, Module::ERROR_FIND_SUBMODULE, Text::MODULE_ERROR_FIND_SUBMODULE, pathToOpen);
      }
      Log::Debug(THIS_MODULE, "Resolved path '%1%'", subpath);
      result.Data = tmpResult.Data;
      result.Path = subpath;
      result.Plugins = tmpResult.Plugins;
    }

    virtual void DetectModules(Parameters::Accessor::Ptr modulesParams, const DetectParameters& detectParams,
      const MetaContainer& data, ModuleRegion& region) const
    {
      Log::Debug(THIS_MODULE, "%3%: Detecting modules in data of size %1%, path '%2%'",
        data.Data->Size(), data.Path, data.Plugins->Count());

      //try to detect container and pass control there
      if (DetectContainer(modulesParams, detectParams, data, region))
      {
        return;
      }

      //try to process implicit
      if (DetectImplicit(modulesParams, detectParams, data, region))
      {
        return;
      }
      //try to detect and process single modules
      DetectModule(modulesParams, detectParams, data, region);
    }

    virtual void OpenModule(Parameters::Accessor::Ptr moduleParams, const MetaContainer& input, Module::Holder::Ptr& holder) const
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
        if (Module::Holder::Ptr module = plugin->CreateModule(moduleParams, input, region))
        {
          Log::Debug(THIS_MODULE, "%2%: Opened player plugin %1%",
            plugin->Id(), input.Plugins->Count());
          holder = module;
          return;
        }
        //TODO: dispatch heavy checks- return false if not enabled
      }
      throw MakeFormattedError(THIS_LINE, Module::ERROR_FIND_SUBMODULE, Text::MODULE_ERROR_FIND_SUBMODULE, input.Path);
    }

  private:
    bool DetectContainer(Parameters::Accessor::Ptr params, const DetectParameters& detectParams, const MetaContainer& input,
      ModuleRegion& region) const
    {
      for (ContainerPluginsArray::const_iterator it = ContainerPlugins.begin(), lim = ContainerPlugins.end();
        it != lim; ++it)
      {
        const ContainerPlugin::Ptr plugin = *it;
        if (detectParams.FilterPlugin(*plugin))
        {
          continue;//filtered plugin
        }
        if (!plugin->Check(*input.Data))
        {
          continue;//invalid plugin
        }
        Log::Debug(THIS_MODULE, "%3%:  Checking container plugin %1% for path '%2%'",
          plugin->Id(), input.Path, input.Plugins->Count());
        if (plugin->Process(params, detectParams, input, region))
        {
          Log::Debug(THIS_MODULE, "%5%:  Container plugin %1% for path '%2%' processed at region (%3%;%4%)",
            plugin->Id(), input.Path, region.Offset, region.Size, input.Plugins->Count());
          return true;
        }
      }
      return false;
    }

    bool DetectImplicit(Parameters::Accessor::Ptr modulesParams, const DetectParameters& detectParams, const MetaContainer& input,
      ModuleRegion& region) const
    {
      LoggerHelper logger(detectParams, input, input.Path.empty() ? Text::MODULE_PROGRESS_DETECT_IMPLICIT_NOPATH : Text::MODULE_PROGRESS_DETECT_IMPLICIT);

      for (ImplicitPluginsArray::const_iterator it = ImplicitPlugins.begin(), lim = ImplicitPlugins.end();
        it != lim; ++it)
      {
        const ImplicitPlugin::Ptr plugin = *it;
        if (detectParams.FilterPlugin(*plugin))
        {
          continue;//filtered plugin
        }
        if (!plugin->Check(*input.Data))
        {
          continue;//invalid plugin
        }
        //find first suitable
        Log::Debug(THIS_MODULE, "%3%:  Checking implicit container %1% at path '%2%'",
          plugin->Id(), input.Path, input.Plugins->Count());
        if (IO::DataContainer::Ptr subdata = plugin->ExtractSubdata(*modulesParams, input, region))
        {
          Log::Debug(THIS_MODULE, "%3%:  Detected at region (%1%;%2%)",
            region.Offset, region.Size, input.Plugins->Count());
          logger(*plugin);

          const String pathComponent = EncodeImplicitPluginToPath(plugin->Id());

          MetaContainer nested;
          nested.Data = subdata;
          nested.Path = IO::AppendPath(input.Path, pathComponent);
          nested.Plugins = input.Plugins->Clone();
          nested.Plugins->Add(plugin);
          ModuleRegion nestedRegion;
          DetectModules(modulesParams, detectParams, nested, nestedRegion);
          return true;
        }
        //TODO: dispatch heavy checks- return false if not enabled
      }
      return false;
    }

    void DetectModule(Parameters::Accessor::Ptr moduleParams, const DetectParameters& detectParams, const MetaContainer& input,
      ModuleRegion& region) const
    {
      LoggerHelper logger(detectParams, input, input.Path.empty() ? Text::MODULE_PROGRESS_DETECT_PLAYER_NOPATH : Text::MODULE_PROGRESS_DETECT_PLAYER);
      for (PlayerPluginsArray::const_iterator it = PlayerPlugins.begin(), lim = PlayerPlugins.end();
        it != lim; ++it)
      {
        const PlayerPlugin::Ptr plugin = *it;
        if (detectParams.FilterPlugin(*plugin))
        {
          continue;//filtered plugin
        }
        if (!plugin->Check(*input.Data))
        {
          continue;//invalid plugin
        }
        Log::Debug(THIS_MODULE, "%3%:  Checking module plugin %1% at path '%2%'",
          plugin->Id(), input.Path, input.Plugins->Count());
        if (Module::Holder::Ptr module = plugin->CreateModule(moduleParams, input, region))
        {
          Log::Debug(THIS_MODULE, "%3%:  Detected at region (%1%;%2%)",
            region.Offset, region.Size, input.Plugins->Count());

          logger(*plugin);

          ThrowIfError(detectParams.ProcessModule(input.Path, module));
          return;
        }
        //TODO: dispatch heavy checks- return false if not enabled
      }
      region.Offset = 0;
      region.Size = 0;
    }

    bool ResolveImplicit(const Parameters::Accessor& commonParams, MetaContainer& data, String& pathToOpen) const
    {
      String restPath;
      const String pathComponent = IO::ExtractFirstPathComponent(pathToOpen, restPath);
      const String pluginId = DecodeImplicitPluginFromPath(pathComponent);
      if (pluginId.empty())
      {
        return false;
      }
      for (ImplicitPluginsArray::const_iterator it = ImplicitPlugins.begin(), lim = ImplicitPlugins.end();
        it != lim; ++it)
      {
        const ImplicitPlugin::Ptr plugin = *it;
        if (plugin->Id() != pluginId)
        {
          continue;
        }
        if (!plugin->Check(*data.Data))
        {
          continue;
        }
        ModuleRegion resRegion;
        if (IO::DataContainer::Ptr subdata = plugin->ExtractSubdata(commonParams, data, resRegion))
        {
          Log::Debug(THIS_MODULE, "Detected implicit plugin %1%", plugin->Id());
          data.Path = IO::AppendPath(data.Path, pathComponent);
          data.Data = subdata;
          data.Plugins->Add(plugin);
          pathToOpen = restPath;
          return true;
        }
        //TODO: dispatch heavy checks- return false if not enabled
      }
      return false;
    }

    bool ResolveContainer(const Parameters::Accessor& commonParams, MetaContainer& data, String& pathToOpen) const
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
          Log::Debug(THIS_MODULE, "Detected nested container %1%", plugin->Id());
          assert(String::npos != pathToOpen.rfind(restPath));
          const String processedPath = pathToOpen.substr(0, pathToOpen.rfind(restPath));
          data.Path += processedPath;
          data.Data = subdata;
          data.Plugins->Add(plugin);
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
  uint_t ModuleRegion::Checksum(const IO::DataContainer& container) const
  {
    const uint8_t* const data = static_cast<const uint8_t*>(container.Data());
    assert(Offset + Size <= container.Size());
    boost::crc_32_type crcCalc;
    crcCalc.process_bytes(data + Offset, Size);
    return crcCalc.checksum();
  }

  IO::DataContainer::Ptr ModuleRegion::Extract(const IO::DataContainer& container) const
  {
    const uint8_t* const data = static_cast<const uint8_t*>(container.Data());
    return IO::CreateDataContainer(data + Offset, Size);
  }

  PluginsChain::Ptr PluginsChain::Create()
  {
    return boost::make_shared<PluginsChainImpl>();
  }

  PluginsEnumerator& PluginsEnumerator::Instance()
  {
    static PluginsEnumeratorImpl instance;
    return instance;
  }

  Plugin::Iterator::Ptr EnumeratePlugins()
  {
    return PluginsEnumerator::Instance().Enumerate();
  }

  Error DetectModules(Parameters::Accessor::Ptr modulesParams, const DetectParameters& detectParams,
    IO::DataContainer::Ptr data, const String& startSubpath)
  {
    if (!data.get())
    {
      return Error(THIS_LINE, Module::ERROR_INVALID_PARAMETERS, Text::MODULE_ERROR_PARAMETERS);
    }
    try
    {
      const PluginsEnumerator& enumerator(PluginsEnumerator::Instance());
      MetaContainer subcontainer;
      enumerator.ResolveSubpath(*modulesParams, data, startSubpath, subcontainer);
      ModuleRegion region;
      enumerator.DetectModules(modulesParams, detectParams, subcontainer, region);
      return Error();
    }
    catch (const Error& e)
    {
      Error err(THIS_LINE, Module::ERROR_DETECT_CANCELED, Text::MODULE_ERROR_CANCELED);
      return err.AddSuberror(e);
    }
    catch (const std::bad_alloc&)
    {
      return Error(THIS_LINE, Module::ERROR_NO_MEMORY, Text::MODULE_ERROR_NO_MEMORY);
    }
  }

  Error OpenModule(Parameters::Accessor::Ptr moduleParams, IO::DataContainer::Ptr data, const String& subpath,
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
      enumerator.ResolveSubpath(*moduleParams, data, subpath, subcontainer);
      //try to detect and process single modules
      enumerator.OpenModule(moduleParams, subcontainer, result);
      return Error();
    }
    catch (const Error& e)
    {
      Error err = MakeFormattedError(THIS_LINE, Module::ERROR_FIND_SUBMODULE, Text::MODULE_ERROR_FIND_SUBMODULE, subpath);
      return err.AddSuberror(e);
    }
    catch (const std::bad_alloc&)
    {
      return Error(THIS_LINE, Module::ERROR_NO_MEMORY, Text::MODULE_ERROR_NO_MEMORY);
    }
  }
}
