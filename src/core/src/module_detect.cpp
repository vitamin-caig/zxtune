/**
* 
* @file
*
* @brief  Module detection logic
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "core/additional_files_resolve.h"
#include "core/plugin_attrs.h"
#include "core/plugins/archive_plugins_enumerator.h"
#include "core/plugins/player_plugins_enumerator.h"
#include "core/src/callback.h"
#include "core/src/l10n.h"
//common includes
#include <contract.h>
#include <error.h>
#include <make_ptr.h>
//library includes
#include <debug/log.h>
#include <module/players/aym/aym_base.h>
#include <parameters/merged_accessor.h>
#include <parameters/container.h>
//std includes
#include <map>
//text includes
#include <src/core/text/core.h>
#include <src/core/text/plugins.h>

#define FILE_TAG 006E56AA

namespace Module
{
  const Debug::Stream DetectDbg("Core::Detection");

  const String ARCHIVE_PLUGIN_PREFIX(Text::ARCHIVE_PLUGIN_PREFIX);

  String EncodeArchivePluginToPath(const String& pluginId)
  {
    return ARCHIVE_PLUGIN_PREFIX + pluginId;
  }

  template<class T>
  std::size_t DetectByPlugins(const Parameters::Accessor& params, typename T::Iterator::Ptr plugins, ZXTune::DataLocation::Ptr location, DetectCallback& callback)
  {
    for (; plugins->IsValid(); plugins->Next())
    {
      const typename T::Ptr plugin = plugins->Get();
      const Analysis::Result::Ptr result = plugin->Detect(params, location, callback);
      if (std::size_t usedSize = result->GetMatchedDataSize())
      {
        DetectDbg("Detected %1% in %2% bytes at %3%.", plugin->GetDescription()->Id(), usedSize, location->GetPath()->AsString());
        return usedSize;
      }
    }
    return 0;
  }
  
  class ResolveAdditionalFilesAdapter : public DetectCallbackDelegate
  {
  public:
    ResolveAdditionalFilesAdapter(const Parameters::Accessor& params, Binary::Container::Ptr data, DetectCallback& delegate)
      : DetectCallbackDelegate(delegate)
      , Source(std::unique_ptr<AdditionalFilesSource>(new FullpathFilesSource(params, std::move(data))))
    {
    }

    void ProcessModule(const ZXTune::DataLocation& location, const ZXTune::Plugin& decoder, Module::Holder::Ptr holder) override
    {
      if (const auto files = dynamic_cast<const Module::AdditionalFiles*>(holder.get()))
      {
        const auto path = location.GetPath();
        if (const auto dir = path->GetParent())
        {
          DetectDbg("Archived multifile %1% at '%2%'", decoder.Id(), path->AsString());
          try
          {
            const ArchivedFilesSource source(dir, Source);
            ResolveAdditionalFiles(source, *files);
          }
          catch (const Error& e)
          {
            DetectDbg(e.ToString().c_str());
            return;
          }
        }
      }
      Delegate.ProcessModule(location, decoder, std::move(holder));
    }
  private:
    class ArchivedFilesSource : public AdditionalFilesSource
    {
    public:
      ArchivedFilesSource(Analysis::Path::Ptr dir, const AdditionalFilesSource& delegate)
        : Dir(std::move(dir))
        , Delegate(delegate)
      {
      }
      
      Binary::Container::Ptr Get(const String& name) const override
      {
        const auto subpath = Dir->Append(name)->AsString();
        DetectDbg("Resolve '%1%' as '%2%'", name, subpath);
        return Delegate.Get(subpath);
      }
    private:
      const Analysis::Path::Ptr Dir;
      const AdditionalFilesSource& Delegate;
    };
    
    class CachedFilesSource : public AdditionalFilesSource
    {
    public:
      CachedFilesSource(std::unique_ptr<const AdditionalFilesSource> delegate)
        : Delegate(std::move(delegate))
      {
      }
      
      Binary::Container::Ptr Get(const String& name) const override
      {
        auto& cached = Cache[name];
        if (!cached)
        {
          cached = Delegate->Get(name);
        }
        else
        {
          DetectDbg("Use cached");
        }
        return cached;
      }
    private:
      const std::unique_ptr<const AdditionalFilesSource> Delegate;
      mutable std::map<String, Binary::Container::Ptr> Cache;
    };
    
    class FullpathFilesSource : public AdditionalFilesSource
    {
    public:
      FullpathFilesSource(const Parameters::Accessor& params, Binary::Container::Ptr data)
        : Params(params)
        , Data(std::move(data))
      {
      }
      
      Binary::Container::Ptr Get(const String& name) const override
      {
        const auto location = ZXTune::OpenLocation(Params, Data, name);
        return location->GetData();
      }
    private:
      const Parameters::Accessor& Params;
      const Binary::Container::Ptr Data;
    };
  private:
    const CachedFilesSource Source;
  };

  //TODO: remove
  class MixedPropertiesHolder : public Holder
  {
  public:
    MixedPropertiesHolder(Holder::Ptr delegate, Parameters::Accessor::Ptr props)
      : Delegate(std::move(delegate))
      , Properties(std::move(props))
    {
    }

    Information::Ptr GetModuleInformation() const override
    {
      return Delegate->GetModuleInformation();
    }

    Parameters::Accessor::Ptr GetModuleProperties() const override
    {
      return Parameters::CreateMergedAccessor(Properties, Delegate->GetModuleProperties());
    }

    Renderer::Ptr CreateRenderer(uint_t samplerate, Parameters::Accessor::Ptr params) const override
    {
      return Delegate->CreateRenderer(samplerate, Parameters::CreateMergedAccessor(params, Properties));
    }
  private:
    const Holder::Ptr Delegate;
    const Parameters::Accessor::Ptr Properties;
  };

  class MixedPropertiesAYMHolder : public AYM::Holder
  {
  public:
    MixedPropertiesAYMHolder(AYM::Holder::Ptr delegate, Parameters::Accessor::Ptr props)
      : Delegate(std::move(delegate))
      , Properties(std::move(props))
    {
    }

    Information::Ptr GetModuleInformation() const override
    {
      return Delegate->GetModuleInformation();
    }

    Parameters::Accessor::Ptr GetModuleProperties() const override
    {
      return Parameters::CreateMergedAccessor(Properties, Delegate->GetModuleProperties());
    }

    Renderer::Ptr CreateRenderer(uint_t samplerate, Parameters::Accessor::Ptr params) const override
    {
      return Delegate->CreateRenderer(samplerate, Parameters::CreateMergedAccessor(params, Properties));
    }

    AYM::Chiptune::Ptr GetChiptune() const override
    {
      return Delegate->GetChiptune();
    }
    
    void Dump(Devices::AYM::Device& dev) const override
    {
      return Delegate->Dump(dev);
    }
  private:
    const AYM::Holder::Ptr Delegate;
    const Parameters::Accessor::Ptr Properties;
  };

  std::size_t OpenInternal(const Parameters::Accessor& params, ZXTune::DataLocation::Ptr location, DetectCallback& callback)
  {
    using namespace ZXTune;
    const PlayerPluginsEnumerator::Ptr usedPlayerPlugins = PlayerPluginsEnumerator::Create();
    return DetectByPlugins<PlayerPlugin>(params, usedPlayerPlugins->Enumerate(), std::move(location), callback);
  }

  class OpenModuleCallback : public DetectCallback
  {
  public:
    explicit OpenModuleCallback(Parameters::Container::Ptr properties)
      : DetectCallback()
      , Properties(std::move(properties))
    {
    }

    Parameters::Container::Ptr CreateInitialProperties(const String& /*subpath*/) const override
    {
      return std::move(Properties);
    }

    void ProcessModule(const ZXTune::DataLocation& /*location*/, const ZXTune::Plugin& /*decoder*/, Module::Holder::Ptr holder) override
    {
      Result = std::move(holder);
    }

    Log::ProgressCallback* GetProgress() const override
    {
      return nullptr;
    }

    Holder::Ptr GetResult() const
    {
      return Result;
    }
  private:
    mutable Parameters::Container::Ptr Properties;
    Holder::Ptr Result;
  };

  void Open(const Parameters::Accessor& params, Binary::Container::Ptr data, const String& subpath, DetectCallback& callback)
  {
    ResolveAdditionalFilesAdapter adapter(params, data, callback);
    auto location = ZXTune::OpenLocation(params, std::move(data), subpath);
    if (!OpenInternal(params, std::move(location), adapter))
    {
      throw Error(THIS_LINE, translate("Failed to find module at specified location."));
    }
  }

  Holder::Ptr Open(const Parameters::Accessor& params, Binary::Container::Ptr data, const String& subpath, Parameters::Container::Ptr initialProperties)
  {
    OpenModuleCallback callback(std::move(initialProperties));
    Open(params, std::move(data), subpath, callback);
    return callback.GetResult();
  }

  Holder::Ptr Open(const Parameters::Accessor& params, const Binary::Container& data, Parameters::Container::Ptr initialProperties)
  {
    using namespace ZXTune;
    for (auto usedPlugins = PlayerPluginsEnumerator::Create()->Enumerate(); usedPlugins->IsValid(); usedPlugins->Next())
    {
      const auto plugin = usedPlugins->Get();
      if (const auto res = plugin->TryOpen(params, data, initialProperties))
      {
        return res;
      }
    }
    throw Error(THIS_LINE, translate("Failed to find module at specified location."));
  }

  std::size_t Detect(const Parameters::Accessor& params, ZXTune::DataLocation::Ptr location, DetectCallback& callback)
  {
    using namespace ZXTune;
    const ArchivePluginsEnumerator::Ptr usedArchivePlugins = ArchivePluginsEnumerator::Create();
    if (std::size_t usedSize = DetectByPlugins<ArchivePlugin>(params, usedArchivePlugins->Enumerate(), location, callback))
    {
      return usedSize;
    }
    return OpenInternal(params, std::move(location), callback);
  }

  void Detect(const Parameters::Accessor& params, Binary::Container::Ptr data, DetectCallback& callback)
  {
    ResolveAdditionalFilesAdapter adapter(params, data, callback);
    Detect(params, ZXTune::CreateLocation(data), adapter);
  }
  
  Holder::Ptr CreateMixedPropertiesHolder(Holder::Ptr delegate, Parameters::Accessor::Ptr props)
  {
    if (const AYM::Holder::Ptr aym = std::dynamic_pointer_cast<const AYM::Holder>(delegate))
    {
      return MakePtr<MixedPropertiesAYMHolder>(aym, props);
    }
    return MakePtr<MixedPropertiesHolder>(delegate, props);
  }
}

#undef FILE_TAG
