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
#include "archives/plugins_list.h"
#include "containers/plugins_list.h"
#include "players/plugins_list.h"
//common includes
#include <error_tools.h>
#include <logging.h>
#include <tools.h>
//library includes
#include <core/error_codes.h>
#include <core/module_attrs.h>
#include <core/module_detect.h>
#include <core/plugin_attrs.h>
#include <io/container.h>
#include <io/fs_tools.h>
//std includes
#include <list>
//boost includes
#include <boost/bind.hpp>
#include <boost/ref.hpp>
#include <boost/bind/apply.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string/join.hpp>
//text includes
#include <core/text/core.h>

#define FILE_TAG 04EDD719

namespace
{
  using namespace ZXTune;

  const std::string THIS_MODULE("Core::Enumerator");

  typedef std::vector<Plugin::Ptr> PluginsArray;
  typedef std::vector<PlayerPlugin::Ptr> PlayerPluginsArray;
  typedef std::vector<ArchivePlugin::Ptr> ArchivePluginsArray;
  typedef std::vector<ContainerPlugin::Ptr> ContainerPluginsArray;

  const Char ROOT_SUBPATH[] = {'/', 0};

  const Char ARCHIVE_PLUGIN_PREFIX[] = {'+', 'u', 'n', 0};

  String EncodeArchivePluginToPath(const String& pluginId)
  {
    return String(ARCHIVE_PLUGIN_PREFIX) + pluginId;
  }

  String DecodeArchivePluginFromPath(const String& path)
  {
    static const String PREFIX(ARCHIVE_PLUGIN_PREFIX);
    return 0 == path.find(PREFIX)
      ? path.substr(PREFIX.size())
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

  template<class PluginType>
  class TypedPluginIterator : public PluginType::Iterator
  {
  public:
    typedef std::vector<typename PluginType::Ptr> TypedPluginsList;
    TypedPluginIterator(typename TypedPluginsList::const_iterator from,
      typename TypedPluginsList::const_iterator to)
      : Pos(from), Limit(to)
    {
    }

    virtual bool IsValid() const
    {
      return Pos != Limit;
    }

    virtual typename PluginType::Ptr Get() const
    {
      //since this implementation is passed to external client, make it as safe as possible
      if (Pos != Limit)
      {
        return *Pos;
      }
      assert(!"Plugin iterator is out of range");
      return typename PluginType::Ptr();
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
    typename TypedPluginsList::const_iterator Pos;
    const typename TypedPluginsList::const_iterator Limit;
  };

  class PluginsChainImpl : public PluginsChain
  {
    typedef std::list<Plugin::Ptr> PluginsList;
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
      return Plugin::Ptr();
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
      RegisterArchivePlugins(*this);
      RegisterPlayerPlugins(*this);
    }

    virtual void RegisterPlugin(PlayerPlugin::Ptr plugin)
    {
      AllPlugins.push_back(plugin);
      PlayerPlugins.push_back(plugin);
      Log::Debug(THIS_MODULE, "Registered player %1%", plugin->Id());
    }

    virtual void RegisterPlugin(ArchivePlugin::Ptr plugin)
    {
      AllPlugins.push_back(plugin);
      ArchivePlugins.push_back(plugin);
      Log::Debug(THIS_MODULE, "Registered archive container %1%", plugin->Id());
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
      return Plugin::Iterator::Ptr(new TypedPluginIterator<Plugin>(AllPlugins.begin(), AllPlugins.end()));
    }

    //private interface
    virtual PlayerPlugin::Iterator::Ptr EnumeratePlayers() const
    {
      return PlayerPlugin::Iterator::Ptr(new TypedPluginIterator<PlayerPlugin>(PlayerPlugins.begin(), PlayerPlugins.end()));
    }

    virtual ArchivePlugin::Iterator::Ptr EnumerateArchives() const
    {
      return ArchivePlugin::Iterator::Ptr(new TypedPluginIterator<ArchivePlugin>(ArchivePlugins.begin(), ArchivePlugins.end()));
    }

    virtual ContainerPlugin::Iterator::Ptr EnumerateContainers() const
    {
      return ContainerPlugin::Iterator::Ptr(new TypedPluginIterator<ContainerPlugin>(ContainerPlugins.begin(), ContainerPlugins.end()));
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
        //check for archives
        if (ResolveArchive(commonParams, tmpResult, pathToOpen))
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

    virtual std::size_t DetectModules(Parameters::Accessor::Ptr modulesParams, const DetectParameters& detectParams,
      const MetaContainer& data) const
    {
      Log::Debug(THIS_MODULE, "%3%: Detecting modules in data of size %1%, path '%2%'",
        data.Data->Size(), data.Path, data.Plugins->Count());

      //try to detect container and pass control there
      if (std::size_t usedSize = DetectContainer(modulesParams, detectParams, data))
      {
        return usedSize;
      }

      //try to process archives
      if (std::size_t usedSize = DetectArchive(modulesParams, detectParams, data))
      {
        return usedSize;
      }
      //try to detect and process single modules
      return DetectModule(modulesParams, detectParams, data);
    }

    virtual Module::Holder::Ptr OpenModule(Parameters::Accessor::Ptr moduleParams, const MetaContainer& input) const
    {
      for (PlayerPluginsArray::const_iterator it = PlayerPlugins.begin(), lim = PlayerPlugins.end();
        it != lim; ++it)
      {
        const PlayerPlugin::Ptr plugin = *it;
        if (!plugin->Check(*input.Data))
        {
          continue;//invalid plugin
        }
        std::size_t usedSize = 0;
        if (Module::Holder::Ptr module = plugin->CreateModule(moduleParams, input, usedSize))
        {
          Log::Debug(THIS_MODULE, "%2%: Opened player plugin %1%",
            plugin->Id(), input.Plugins->Count());
          return module;
        }
        //TODO: dispatch heavy checks- return false if not enabled
      }
      throw MakeFormattedError(THIS_LINE, Module::ERROR_FIND_SUBMODULE, Text::MODULE_ERROR_FIND_SUBMODULE, input.Path);
    }

  private:
    std::size_t DetectContainer(Parameters::Accessor::Ptr params, const DetectParameters& detectParams, const MetaContainer& input) const
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
        if (std::size_t usedSize = plugin->Process(params, detectParams, input))
        {
          Log::Debug(THIS_MODULE, "%4%:  Container plugin %1% for path '%2%' processed at size %3%",
            plugin->Id(), input.Path, usedSize, input.Plugins->Count());
          return usedSize;
        }
      }
      return 0;
    }

    std::size_t DetectArchive(Parameters::Accessor::Ptr modulesParams, const DetectParameters& detectParams, const MetaContainer& input) const
    {
      LoggerHelper logger(detectParams, input, input.Path.empty() ? Text::MODULE_PROGRESS_DETECT_ARCHIVE_NOPATH : Text::MODULE_PROGRESS_DETECT_ARCHIVE);

      for (ArchivePluginsArray::const_iterator it = ArchivePlugins.begin(), lim = ArchivePlugins.end();
        it != lim; ++it)
      {
        const ArchivePlugin::Ptr plugin = *it;
        if (detectParams.FilterPlugin(*plugin))
        {
          continue;//filtered plugin
        }
        const IO::DataContainer& dataContainer = *input.Data;
        if (!plugin->Check(dataContainer))
        {
          continue;//invalid plugin
        }
        //find first suitable
        Log::Debug(THIS_MODULE, "%3%:  Checking archive %1% at path '%2%'",
          plugin->Id(), input.Path, input.Plugins->Count());
        std::size_t usedSize = 0;
        if (IO::DataContainer::Ptr subdata = plugin->ExtractSubdata(*modulesParams, dataContainer, usedSize))
        {
          Log::Debug(THIS_MODULE, "%2%:  Detected at data size %1%",
            usedSize, input.Plugins->Count());
          logger(*plugin);

          const String pathComponent = EncodeArchivePluginToPath(plugin->Id());

          MetaContainer nested;
          nested.Data = subdata;
          nested.Path = IO::AppendPath(input.Path, pathComponent);
          nested.Plugins = input.Plugins->Clone();
          nested.Plugins->Add(plugin);
          DetectModules(modulesParams, detectParams, nested);
          return usedSize;
        }
        Log::Debug(THIS_MODULE, "%1%:  Failed to detect", input.Plugins->Count());
        //TODO: dispatch heavy checks- return false if not enabled
      }
      return 0;
    }

    std::size_t DetectModule(Parameters::Accessor::Ptr moduleParams, const DetectParameters& detectParams, const MetaContainer& input) const
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
        std::size_t usedSize = 0;
        if (Module::Holder::Ptr module = plugin->CreateModule(moduleParams, input, usedSize))
        {
          Log::Debug(THIS_MODULE, "%2%:  Detected at size %1%",
            usedSize, input.Plugins->Count());

          logger(*plugin);

          ThrowIfError(detectParams.ProcessModule(input.Path, module));
          return usedSize;
        }
        Log::Debug(THIS_MODULE, "%1%:  Failed to detect", input.Plugins->Count());
        //TODO: dispatch heavy checks- return false if not enabled
      }
      return 0;
    }

    bool ResolveArchive(const Parameters::Accessor& commonParams, MetaContainer& data, String& pathToOpen) const
    {
      String restPath;
      const String pathComponent = IO::ExtractFirstPathComponent(pathToOpen, restPath);
      const String pluginId = DecodeArchivePluginFromPath(pathComponent);
      if (pluginId.empty())
      {
        return false;
      }
      for (ArchivePluginsArray::const_iterator it = ArchivePlugins.begin(), lim = ArchivePlugins.end();
        it != lim; ++it)
      {
        const ArchivePlugin::Ptr plugin = *it;
        if (plugin->Id() != pluginId)
        {
          continue;
        }
        const IO::DataContainer& dataContainer = *data.Data;
        if (!plugin->Check(dataContainer))
        {
          continue;
        }
        std::size_t usedSize = 0;
        if (IO::DataContainer::Ptr subdata = plugin->ExtractSubdata(commonParams, dataContainer, usedSize))
        {
          Log::Debug(THIS_MODULE, "Detected archive plugin %1%", plugin->Id());
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
    PluginsArray AllPlugins;
    ContainerPluginsArray ContainerPlugins;
    ArchivePluginsArray ArchivePlugins;
    PlayerPluginsArray PlayerPlugins;
  };
}

namespace ZXTune
{
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
      enumerator.DetectModules(modulesParams, detectParams, subcontainer);
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
      result = enumerator.OpenModule(moduleParams, subcontainer);
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
