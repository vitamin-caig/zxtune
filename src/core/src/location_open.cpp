/**
* 
* @file
*
* @brief  Location open and resolving implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "location.h"
#include "core/plugins/archive_plugins_enumerator.h"
//common includes
#include <error_tools.h>
#include <make_ptr.h>
//library includes
#include <debug/log.h>
#include <l10n/api.h>
//text includes
#include <src/core/text/core.h>

#define FILE_TAG BCCC5654

namespace
{
  using namespace ZXTune;

  const Debug::Stream Dbg("Core");
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("core");

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

  class GeneratedLocation : public DataLocation
  {
  public:
    GeneratedLocation(Binary::Container::Ptr data, const String& plugin, const String& path)
      : Data(data)
      , Path(Analysis::ParsePath(path, Text::MODULE_SUBPATH_DELIMITER[0]))
      , Plugins(Analysis::ParsePath(plugin, Text::MODULE_CONTAINERS_DELIMITER[0]))
    {
    }

    virtual Binary::Container::Ptr GetData() const
    {
      return Data;
    }

    virtual Analysis::Path::Ptr GetPath() const
    {
      return Path;
    }

    virtual Analysis::Path::Ptr GetPluginsChain() const
    {
      return Plugins;
    }
  private:
    const Binary::Container::Ptr Data;
    const Analysis::Path::Ptr Path;
    const Analysis::Path::Ptr Plugins;
  };

  DataLocation::Ptr TryToOpenLocation(const ArchivePluginsEnumerator& plugins, const Parameters::Accessor& params, DataLocation::Ptr location, const Analysis::Path& subPath)
  {
    for (ArchivePlugin::Iterator::Ptr iter = plugins.Enumerate(); iter->IsValid(); iter->Next())
    {
      const ArchivePlugin::Ptr plugin = iter->Get();
      if (DataLocation::Ptr result = plugin->Open(params, location, subPath))
      {
        return result;
      }
    }
    return DataLocation::Ptr();
  }
}

namespace ZXTune
{
  DataLocation::Ptr CreateLocation(Binary::Container::Ptr data)
  {
    return MakePtr<UnresolvedLocation>(data);
  }

  DataLocation::Ptr OpenLocation(const Parameters::Accessor& params, Binary::Container::Ptr data, const String& subpath)
  {
    const ArchivePluginsEnumerator::Ptr usedPlugins = ArchivePluginsEnumerator::Create();
    DataLocation::Ptr resolvedLocation = MakePtr<UnresolvedLocation>(data);
    const Analysis::Path::Ptr sourcePath = Analysis::ParsePath(subpath, Text::MODULE_SUBPATH_DELIMITER[0]);
    for (Analysis::Path::Ptr unresolved = sourcePath; !unresolved->Empty(); unresolved = sourcePath->Extract(resolvedLocation->GetPath()->AsString()))
    {
      const String toResolve = unresolved->AsString();
      Dbg("Resolving '%1%'", toResolve);
      if (!(resolvedLocation = TryToOpenLocation(*usedPlugins, params, resolvedLocation, *unresolved)))
      {
        throw MakeFormattedError(THIS_LINE, translate("Failed to resolve subpath '%1%'."), subpath);
      }
    }
    Dbg("Resolved '%1%'", subpath);
    return resolvedLocation;
  }

  DataLocation::Ptr CreateLocation(Binary::Container::Ptr data, const String& plugin, const String& path)
  {
    return MakePtr<GeneratedLocation>(data, plugin, path);
  }
}
