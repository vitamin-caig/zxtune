/*
Abstract:
  Module location used for opening and resolving

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "location.h"
#include "core/plugins/enumerator.h"
//common includes
#include <logging.h>
#include <tools.h>
//boost includes
#include <boost/make_shared.hpp>

namespace
{
  using namespace ZXTune;

  const std::string THIS_MODULE("Core");

  class EmptyPluginsChain : public PluginsChain
  {
    EmptyPluginsChain()
    {
    }
  public:
    static PluginsChain::Ptr Create()
    {
      static EmptyPluginsChain instance;
      return PluginsChain::Ptr(&instance, NullDeleter<EmptyPluginsChain>());
    }

    virtual uint_t Count() const
    {
      return 0;
    }

    virtual String AsString() const
    {
      return String();
    }
  };

  class UnresolvedLocation : public DataLocation
  {
  public:
    explicit UnresolvedLocation(IO::DataContainer::Ptr data)
      : Data(data)
    {
    }

    virtual IO::DataContainer::Ptr GetData() const
    {
      return Data;
    }

    virtual String GetPath() const
    {
      return String();
    }

    virtual PluginsChain::Ptr GetPlugins() const
    {
      return EmptyPluginsChain::Create();
    }
  private:
    const IO::DataContainer::Ptr Data;
  };

  DataLocation::Ptr Resolve(const PluginsEnumerator& plugins, const Parameters::Accessor& coreParams, DataLocation::Ptr location, const String& pathToResolve)
  {
    for (ArchivePlugin::Iterator::Ptr iter = plugins.EnumerateArchives(); iter->IsValid(); iter->Next())
    {
      const ArchivePlugin::Ptr plugin = iter->Get();
      if (DataLocation::Ptr result = plugin->Open(coreParams, location, pathToResolve))
      {
        return result;
      }
    }
    for (ContainerPlugin::Iterator::Ptr iter = plugins.EnumerateContainers(); iter->IsValid(); iter->Next())
    {
      const ContainerPlugin::Ptr plugin = iter->Get();
      if (DataLocation::Ptr result = plugin->Open(coreParams, location, pathToResolve))
      {
        return result;
      }
    }
    return DataLocation::Ptr();
  }
}

namespace ZXTune
{
  DataLocation::Ptr CreateLocation(Parameters::Accessor::Ptr /*coreParams*/, IO::DataContainer::Ptr data)
  {
    return boost::make_shared<UnresolvedLocation>(data);
  }

  DataLocation::Ptr OpenLocation(Parameters::Accessor::Ptr coreParams, IO::DataContainer::Ptr data, const String& subpath)
  {
    const DataLocation::Ptr initialLocation = boost::make_shared<UnresolvedLocation>(data);
    if (subpath.empty())
    {
      return initialLocation;
    }

    String pathToResolve = subpath;
    const PluginsEnumerator::Ptr usedPlugins = PluginsEnumerator::Create();
    DataLocation::Ptr resolvedLocation = initialLocation;
    while (!pathToResolve.empty())
    {
      Log::Debug(THIS_MODULE, "Resolving '%1%'", pathToResolve);
      if (const DataLocation::Ptr loc = Resolve(*usedPlugins, *coreParams, resolvedLocation, pathToResolve))
      {
        resolvedLocation = loc;
        const String resolvedPath = resolvedLocation->GetPath();
        if (resolvedPath == subpath)
        {
          break;
        }
        assert(0 == subpath.find(resolvedPath));
        pathToResolve = subpath.substr(resolvedPath.size() + 1);
      }
      else
      {
        Log::Debug(THIS_MODULE, "Failed to resolve subpath '%1%'", pathToResolve);
        return DataLocation::Ptr();
      }
    }
    Log::Debug(THIS_MODULE, "Resolved '%1%'", resolvedLocation->GetPath());
    return resolvedLocation;
  }
}
