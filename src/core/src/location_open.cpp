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

  class EmptyDataPath : public DataPath
  {
    EmptyDataPath()
    {
    }
  public:
    static Ptr Create()
    {
      static EmptyDataPath instance;
      return Ptr(&instance, NullDeleter<EmptyDataPath>());
    }

    virtual String AsString() const
    {
      return String();
    }

    virtual String GetFirstComponent() const
    {
      return String();
    }
  };

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

    virtual Plugin::Ptr GetLast() const
    {
      return Plugin::Ptr();
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

    virtual DataPath::Ptr GetPath() const
    {
      return EmptyDataPath::Create();
    }

    virtual PluginsChain::Ptr GetPlugins() const
    {
      return EmptyPluginsChain::Create();
    }
  private:
    const IO::DataContainer::Ptr Data;
  };

  DataLocation::Ptr TryToOpenLocation(const PluginsEnumerator& plugins, const Parameters::Accessor& coreParams, DataLocation::Ptr location, const DataPath& subPath)
  {
    for (ArchivePlugin::Iterator::Ptr iter = plugins.EnumerateArchives(); iter->IsValid(); iter->Next())
    {
      const ArchivePlugin::Ptr plugin = iter->Get();
      if (DataLocation::Ptr result = plugin->Open(coreParams, location, subPath))
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

    const DataPath::Ptr pathToResolve = CreateDataPath(subpath);
    const PluginsEnumerator::Ptr usedPlugins = PluginsEnumerator::Create();
    DataLocation::Ptr resolvedLocation = initialLocation;
    for (DataPath::Ptr unresolved = pathToResolve; unresolved; unresolved = SubstractDataPath(*pathToResolve, *resolvedLocation->GetPath()))
    {
      const String toResolve = unresolved->AsString();
      Log::Debug(THIS_MODULE, "Resolving '%1%'", toResolve);
      if (!(resolvedLocation = TryToOpenLocation(*usedPlugins, *coreParams, resolvedLocation, *unresolved)))
      {
        Log::Debug(THIS_MODULE, "Failed to resolve subpath '%1%'", toResolve);
        return DataLocation::Ptr();
      }
    }
    Log::Debug(THIS_MODULE, "Resolved '%1%'", resolvedLocation->GetPath()->AsString());
    return resolvedLocation;
  }
}
