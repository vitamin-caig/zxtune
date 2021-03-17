/**
 *
 * @file
 *
 * @brief  Location open and resolving implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "core/plugins/archive_plugins_enumerator.h"
#include "core/src/l10n.h"
#include "core/src/location.h"
// common includes
#include <error_tools.h>
#include <make_ptr.h>
// library includes
#include <debug/log.h>
#include <module/attributes.h>

#define FILE_TAG BCCC5654

namespace ZXTune
{
  const Debug::Stream Dbg("Core");
  using Module::translate;

  Analysis::Path::Ptr CreateEmptyPath()
  {
    static const Analysis::Path::Ptr instance = Analysis::ParsePath(String(), Module::SUBPATH_DELIMITER);
    return instance;
  }

  Analysis::Path::Ptr CreateEmptyPluginsChain()
  {
    static const Analysis::Path::Ptr instance = Analysis::ParsePath(String(), Module::CONTAINERS_DELIMITER);
    return instance;
  }

  class UnresolvedLocation : public DataLocation
  {
  public:
    explicit UnresolvedLocation(Binary::Container::Ptr data)
      : Data(std::move(data))
    {}

    Binary::Container::Ptr GetData() const override
    {
      return Data;
    }

    Analysis::Path::Ptr GetPath() const override
    {
      return CreateEmptyPath();
    }

    Analysis::Path::Ptr GetPluginsChain() const override
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
      : Data(std::move(data))
      , Path(Analysis::ParsePath(path, Module::SUBPATH_DELIMITER))
      , Plugins(Analysis::ParsePath(plugin, Module::CONTAINERS_DELIMITER))
    {}

    Binary::Container::Ptr GetData() const override
    {
      return Data;
    }

    Analysis::Path::Ptr GetPath() const override
    {
      return Path;
    }

    Analysis::Path::Ptr GetPluginsChain() const override
    {
      return Plugins;
    }

  private:
    const Binary::Container::Ptr Data;
    const Analysis::Path::Ptr Path;
    const Analysis::Path::Ptr Plugins;
  };

  DataLocation::Ptr TryToOpenLocation(const ArchivePluginsEnumerator& plugins, const Parameters::Accessor& params,
                                      DataLocation::Ptr location, const Analysis::Path& subPath)
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
}  // namespace ZXTune

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
    const Analysis::Path::Ptr sourcePath = Analysis::ParsePath(subpath, Module::SUBPATH_DELIMITER);
    for (Analysis::Path::Ptr unresolved = sourcePath; !unresolved->Empty();
         unresolved = sourcePath->Extract(resolvedLocation->GetPath()->AsString()))
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
}  // namespace ZXTune

#undef FILE_TAG
