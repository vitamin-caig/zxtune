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
//text includes
#include <src/core/text/core.h>

namespace
{
  using namespace ZXTune;

  const std::string THIS_MODULE("Core");

  Analysis::Path::Ptr CreateEmptyPath()
  {
    static const Analysis::Path::Ptr instance = Analysis::ParsePath(String(), Text::MODULE_SUBPATH_DELIMITER[0]);
    return instance;
  }

  Analysis::Path::Ptr CreateEmptyPluginsChain()
  {
    static const Analysis::Path::Ptr instance = Analysis::ParsePath(String(), Text::MODULE_CONTAINERS_DELIMITER[0]);
    return instance;
  }

  class UnresolvedLocation : public DataLocation
  {
  public:
    explicit UnresolvedLocation(Binary::Container::Ptr data)
      : Data(data)
    {
    }

    virtual Binary::Container::Ptr GetData() const
    {
      return Data;
    }

    virtual Analysis::Path::Ptr GetPath() const
    {
      return CreateEmptyPath();
    }

    virtual Analysis::Path::Ptr GetPluginsChain() const
    {
      return CreateEmptyPluginsChain();
    }
  private:
    const Binary::Container::Ptr Data;
  };

  DataLocation::Ptr TryToOpenLocation(const PluginsEnumerator& plugins, const Parameters::Accessor& coreParams, DataLocation::Ptr location, const Analysis::Path& subPath)
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
  DataLocation::Ptr CreateLocation(Parameters::Accessor::Ptr /*coreParams*/, Binary::Container::Ptr data)
  {
    return boost::make_shared<UnresolvedLocation>(data);
  }

  DataLocation::Ptr OpenLocation(Parameters::Accessor::Ptr coreParams, Binary::Container::Ptr data, const String& subpath)
  {
    const PluginsEnumerator::Ptr usedPlugins = PluginsEnumerator::Create();
    DataLocation::Ptr resolvedLocation = boost::make_shared<UnresolvedLocation>(data);
    const Analysis::Path::Ptr sourcePath = Analysis::ParsePath(subpath, Text::MODULE_SUBPATH_DELIMITER[0]);
    for (Analysis::Path::Ptr unresolved = sourcePath; !unresolved->Empty(); unresolved = sourcePath->Extract(resolvedLocation->GetPath()->AsString()))
    {
      const String toResolve = unresolved->AsString();
      Log::Debug(THIS_MODULE, "Resolving '%1%'", toResolve);
      if (!(resolvedLocation = TryToOpenLocation(*usedPlugins, *coreParams, resolvedLocation, *unresolved)))
      {
        Log::Debug(THIS_MODULE, "Failed to resolve subpath '%1%'", toResolve);
        return DataLocation::Ptr();
      }
    }
    Log::Debug(THIS_MODULE, "Resolved '%1%'", subpath);
    return resolvedLocation;
  }
}
