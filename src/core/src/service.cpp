/**
 *
 * @file
 *
 * @brief  Core service implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "core/plugins/archive_plugin.h"
#include "core/plugins/player_plugin.h"
#include "core/src/callback.h"
#include "core/src/l10n.h"
#include "core/src/location.h"
// common includes
#include <error_tools.h>
#include <make_ptr.h>
// library includes
#include <core/additional_files_resolve.h>
#include <core/service.h>
#include <debug/log.h>
#include <module/attributes.h>
// std includes
#include <map>

namespace ZXTune
{
  const Debug::Stream Dbg("Core::Service");
  using Module::translate;

  class LocationSource
  {
  public:
    virtual ~LocationSource() = default;

    virtual DataLocation::Ptr OpenLocation(Binary::Container::Ptr data, const String& subpath) const = 0;
  };

  class ResolveAdditionalFilesAdapter : public Module::DetectCallbackDelegate
  {
  public:
    ResolveAdditionalFilesAdapter(const LocationSource& src, Binary::Container::Ptr data,
                                  Module::DetectCallback& delegate)
      : DetectCallbackDelegate(delegate)
      , Source(std::make_unique<FullpathFilesSource>(src, std::move(data)))
    {}

    void ProcessModule(const DataLocation& location, const Plugin& decoder, Module::Holder::Ptr holder) override
    {
      if (const auto files = dynamic_cast<const Module::AdditionalFiles*>(holder.get()))
      {
        const auto path = location.GetPath();
        if (const auto dir = path->GetParent())
        {
          Dbg("Archived multifile {} at '{}'", decoder.Id(), path->AsString());
          try
          {
            const ArchivedFilesSource source(dir, Source);
            ResolveAdditionalFiles(source, *files);
          }
          catch (const Error& e)
          {
            Dbg(e.ToString().c_str());
            Delegate.ProcessUnknownData(location);
            return;
          }
        }
      }
      Delegate.ProcessModule(location, decoder, std::move(holder));
    }

  private:
    class ArchivedFilesSource : public Module::AdditionalFilesSource
    {
    public:
      ArchivedFilesSource(Analysis::Path::Ptr dir, const Module::AdditionalFilesSource& delegate)
        : Dir(std::move(dir))
        , Delegate(delegate)
      {}

      Binary::Container::Ptr Get(const String& name) const override
      {
        const auto subpath = Dir->Append(name)->AsString();
        Dbg("Resolve '{}' as '{}'", name, subpath);
        return Delegate.Get(subpath);
      }

    private:
      const Analysis::Path::Ptr Dir;
      const AdditionalFilesSource& Delegate;
    };

    class CachedFilesSource : public Module::AdditionalFilesSource
    {
    public:
      CachedFilesSource(std::unique_ptr<const Module::AdditionalFilesSource> delegate)
        : Delegate(std::move(delegate))
      {}

      Binary::Container::Ptr Get(const String& name) const override
      {
        auto& cached = Cache[name];
        if (!cached)
        {
          cached = Delegate->Get(name);
        }
        else
        {
          Dbg("Use cached");
        }
        return cached;
      }

    private:
      const std::unique_ptr<const AdditionalFilesSource> Delegate;
      mutable std::map<String, Binary::Container::Ptr> Cache;
    };

    class FullpathFilesSource : public Module::AdditionalFilesSource
    {
    public:
      FullpathFilesSource(const LocationSource& src, Binary::Container::Ptr data)
        : Src(src)
        , Data(std::move(data))
      {}

      Binary::Container::Ptr Get(const String& name) const override
      {
        const auto location = Src.OpenLocation(Data, name);
        return location->GetData();
      }

    private:
      const LocationSource& Src;
      const Binary::Container::Ptr Data;
    };

  private:
    const CachedFilesSource Source;
  };

  class OpenModuleCallback : public Module::DetectCallback
  {
  public:
    explicit OpenModuleCallback(Parameters::Container::Ptr properties)
      : DetectCallback()
      , Properties(std::move(properties))
    {}

    Parameters::Container::Ptr CreateInitialProperties(const String& /*subpath*/) const override
    {
      // May be called multiple times, so do not destruct
      return Properties;
    }

    void ProcessModule(const DataLocation& /*location*/, const Plugin& /*decoder*/, Module::Holder::Ptr holder) override
    {
      Result = std::move(holder);
    }

    Log::ProgressCallback* GetProgress() const override
    {
      return nullptr;
    }

    Module::Holder::Ptr GetResult() const
    {
      return Result;
    }

  private:
    mutable Parameters::Container::Ptr Properties;
    Module::Holder::Ptr Result;
  };

  class ServiceImpl
    : public Service
    , private LocationSource
  {
  public:
    explicit ServiceImpl(Parameters::Accessor::Ptr params)
      : Params(std::move(params))
    {}

    Binary::Container::Ptr OpenData(Binary::Container::Ptr data, const String& subpath) const override
    {
      return OpenLocation(std::move(data), subpath)->GetData();
    }

    Module::Holder::Ptr OpenModule(Binary::Container::Ptr data, const String& subpath,
                                   Parameters::Container::Ptr initialProperties) const override
    {
      if (subpath.empty())
      {
        return OpenModule(*data, std::move(initialProperties));
      }
      else
      {
        OpenModuleCallback callback(std::move(initialProperties));
        OpenModule(std::move(data), subpath, callback);
        return callback.GetResult();
      }
    }

    void DetectModules(Binary::Container::Ptr data, Module::DetectCallback& callback) const override
    {
      ResolveAdditionalFilesAdapter adapter(*this, data, callback);
      DetectModules(CreateLocation(data), adapter);
    }

    void OpenModule(Binary::Container::Ptr data, const String& subpath, Module::DetectCallback& callback) const override
    {
      ResolveAdditionalFilesAdapter adapter(*this, data, callback);
      auto location = OpenLocation(std::move(data), subpath);
      if (!DetectBy(PlayerPlugin::Enumerate(), std::move(location), adapter))
      {
        throw Error(THIS_LINE, translate("Failed to find module at specified location."));
      }
    }

  private:
    DataLocation::Ptr OpenLocation(Binary::Container::Ptr data, const String& subpath) const override
    {
      auto resolvedLocation = CreateLocation(std::move(data));
      const auto sourcePath = Analysis::ParsePath(subpath, Module::SUBPATH_DELIMITER);
      for (auto unresolved = sourcePath; !unresolved->Empty();)
      {
        Dbg("Resolving '{}'", unresolved->AsString());
        resolvedLocation = TryToOpenLocation(resolvedLocation, *unresolved);
        if (resolvedLocation)
        {
          unresolved = sourcePath->Extract(resolvedLocation->GetPath()->AsString());
          if (unresolved)
          {
            continue;
          }
        }
        throw MakeFormattedError(THIS_LINE, translate("Failed to resolve subpath '{}'."), subpath);
      }
      Dbg("Resolved '{}'", subpath);
      return resolvedLocation;
    }

    DataLocation::Ptr TryToOpenLocation(DataLocation::Ptr location, const Analysis::Path& subPath) const
    {
      for (const auto& plugin : ArchivePlugin::Enumerate())
      {
        if (auto result = plugin->TryOpen(*Params, location, subPath))
        {
          return result;
        }
      }
      return {};
    }

    Module::Holder::Ptr OpenModule(const Binary::Container& data, Parameters::Container::Ptr initialProperties) const
    {
      for (const auto& plugin : PlayerPlugin::Enumerate())
      {
        if (auto res = plugin->TryOpen(*Params, data, initialProperties))
        {
          return res;
        }
      }
      throw Error(THIS_LINE, translate("Failed to find module at specified location."));
    }

    void DetectModules(DataLocation::Ptr location, Module::DetectCallback& callback) const
    {
      if (!DetectInArchives(location, callback) && !DetectBy(PlayerPlugin::Enumerate(), location, callback))
      {
        callback.ProcessUnknownData(*location);
      }
    }

    class RecursiveDetectionAdapter : public ArchiveCallback
    {
    public:
      RecursiveDetectionAdapter(const ServiceImpl& svc, Module::DetectCallback& delegate, bool useProgress)
        : Svc(svc)
        , Delegate(delegate)
        , Progress(useProgress ? Delegate.GetProgress() : nullptr)
      {}
      Parameters::Container::Ptr CreateInitialProperties(const String& subpath) const override
      {
        return Delegate.CreateInitialProperties(subpath);
      }

      void ProcessModule(const DataLocation& location, const Plugin& decoder, Module::Holder::Ptr holder) override
      {
        Delegate.ProcessModule(location, decoder, std::move(holder));
      }

      void ProcessUnknownData(const ZXTune::DataLocation& location) override
      {
        Delegate.ProcessUnknownData(location);
      }

      Log::ProgressCallback* GetProgress() const override
      {
        return Progress;
      }

      void ProcessData(DataLocation::Ptr data) override
      {
        // TODO: proper progress
        Svc.DetectModules(std::move(data), Delegate);
      }

    private:
      const ServiceImpl& Svc;
      Module::DetectCallback& Delegate;
      Log::ProgressCallback* const Progress;
    };

    bool DetectInArchives(DataLocation::Ptr location, Module::DetectCallback& callback) const
    {
      // Track progress only for top-level container
      const auto useProgress = location->GetPath()->Empty();
      RecursiveDetectionAdapter adapter(*this, callback, useProgress);
      return DetectBy(ArchivePlugin::Enumerate(), std::move(location), adapter);
    }

    template<class PluginsSet, class CallbackType>
    std::size_t DetectBy(const PluginsSet& pluginsSet, DataLocation::Ptr location, CallbackType& callback) const
    {
      for (const auto& plugin : pluginsSet)
      {
        const auto result = plugin->Detect(*Params, location, callback);
        if (auto usedSize = result->GetMatchedDataSize())
        {
          Dbg("Detected {} in {} bytes at {}.", plugin->Id(), usedSize, location->GetPath()->AsString());
          return usedSize;
        }
      }
      return 0;
    }

  private:
    const Parameters::Accessor::Ptr Params;
  };

  Service::Ptr Service::Create(Parameters::Accessor::Ptr parameters)
  {
    return MakePtr<ServiceImpl>(std::move(parameters));
  }
}  // namespace ZXTune

