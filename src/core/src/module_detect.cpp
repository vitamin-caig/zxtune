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
#include "callback.h"
#include "core/additional_files_resolve.h"
#include "core/plugin_attrs.h"
#include "core/plugins/archive_plugins_enumerator.h"
#include "core/plugins/player_plugins_enumerator.h"
//common includes
#include <contract.h>
#include <error.h>
#include <make_ptr.h>
//library includes
#include <debug/log.h>
#include <l10n/api.h>
#include <module/players/aym/aym_base.h>
#include <parameters/merged_accessor.h>
#include <parameters/container.h>
//text includes
#include <src/core/text/core.h>
#include <src/core/text/plugins.h>

#define FILE_TAG 006E56AA

namespace Module
{
  const Debug::Stream Dbg("Core::Detection");
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("core");

  const String ARCHIVE_PLUGIN_PREFIX(Text::ARCHIVE_PLUGIN_PREFIX);

  String EncodeArchivePluginToPath(const String& pluginId)
  {
    return ARCHIVE_PLUGIN_PREFIX + pluginId;
  }

  class OpenModuleCallback : public DetectCallback
  {
  public:
    OpenModuleCallback()
      : DetectCallback()
    {
    }

    void ProcessModule(ZXTune::DataLocation::Ptr /*location*/, ZXTune::Plugin::Ptr /*decoder*/, Module::Holder::Ptr holder) const override
    {
      Result = holder;
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
    mutable Holder::Ptr Result;
  };
  
  template<class T>
  std::size_t DetectByPlugins(const Parameters::Accessor& params, typename T::Iterator::Ptr plugins, ZXTune::DataLocation::Ptr location, const DetectCallback& callback)
  {
    for (; plugins->IsValid(); plugins->Next())
    {
      const typename T::Ptr plugin = plugins->Get();
      const Analysis::Result::Ptr result = plugin->Detect(params, location, callback);
      if (std::size_t usedSize = result->GetMatchedDataSize())
      {
        Dbg("Detected %1% in %2% bytes at %3%.", plugin->GetDescription()->Id(), usedSize, location->GetPath()->AsString());
        return usedSize;
      }
    }
    return 0;
  }
  
  class ResolveAdditionalFilesAdapter : public DetectCallback
  {
  public:
    ResolveAdditionalFilesAdapter(const Parameters::Accessor& params, Binary::Container::Ptr data, const DetectCallback& delegate)
      : Params(params)
      , Data(std::move(data))
      , Delegate(delegate)
    {
    }

    void ProcessModule(ZXTune::DataLocation::Ptr location, ZXTune::Plugin::Ptr decoder, Module::Holder::Ptr holder) const override
    {
      if (const auto files = dynamic_cast<const Module::AdditionalFiles*>(holder.get()))
      {
        const auto path = location->GetPath();
        if (const auto dir = path->GetParent())
        {
          Dbg("Archived multifile %1% at '%2%'", decoder->Id(), path->AsString());
          try
          {
            const ArchivedFilesSource source(dir, Params, Data);
            ResolveAdditionalFiles(source, *files);
          }
          catch (const Error& e)
          {
            Dbg(e.ToString().c_str());
            return;
          }
        }
      }
      Delegate.ProcessModule(std::move(location), std::move(decoder), std::move(holder));
    }

    Log::ProgressCallback* GetProgress() const override
    {
      return Delegate.GetProgress();
    }
  private:
    class ArchivedFilesSource : public AdditionalFilesSource
    {
    public:
      ArchivedFilesSource(Analysis::Path::Ptr dir, const Parameters::Accessor& params, Binary::Container::Ptr data)
        : Dir(std::move(dir))
        , Params(params)
        , Data(std::move(data))
      {
      }
      
      Binary::Container::Ptr Get(const String& name) const override
      {
        const auto subpath = Dir->Append(name)->AsString();
        Dbg("Resolve '%1%' as '%2%'", name, subpath);
        const auto location = ZXTune::OpenLocation(Params, Data, subpath);
        return location->GetData();
      }
    private:
      const Analysis::Path::Ptr Dir;
      const Parameters::Accessor& Params;
      const Binary::Container::Ptr Data;
    };
  private:
    const Parameters::Accessor& Params;
    const Binary::Container::Ptr Data;
    const DetectCallback& Delegate;
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

    Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target) const override
    {
      return Delegate->CreateRenderer(Parameters::CreateMergedAccessor(params, Properties), target);
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

    Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target) const override
    {
      return Delegate->CreateRenderer(Parameters::CreateMergedAccessor(params, Properties), target);
    }

    Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Devices::AYM::Device::Ptr chip) const override
    {
      return Delegate->CreateRenderer(Parameters::CreateMergedAccessor(params, Properties), chip);
    }

    AYM::Chiptune::Ptr GetChiptune() const override
    {
      return Delegate->GetChiptune();
    }
  private:
    const AYM::Holder::Ptr Delegate;
    const Parameters::Accessor::Ptr Properties;
  };

  std::size_t OpenInternal(const Parameters::Accessor& params, ZXTune::DataLocation::Ptr location, const DetectCallback& callback)
  {
    using namespace ZXTune;
    const PlayerPluginsEnumerator::Ptr usedPlayerPlugins = PlayerPluginsEnumerator::Create();
    return DetectByPlugins<PlayerPlugin>(params, usedPlayerPlugins->Enumerate(), location, callback);
  }

  void Open(const Parameters::Accessor& params, Binary::Container::Ptr data, const String& subpath, const DetectCallback& callback)
  {
    const ResolveAdditionalFilesAdapter adapter(params, data, callback);
    const auto location = ZXTune::OpenLocation(params, data, subpath);
    if (!OpenInternal(params, location, adapter))
    {
      throw Error(THIS_LINE, translate("Failed to find module at specified location."));
    }
  }
  
  Holder::Ptr Open(const Parameters::Accessor& params, Binary::Container::Ptr data, const String& subpath)
  {
    const OpenModuleCallback callback;
    Open(params, data, subpath, callback);
    return callback.GetResult();
  }

  Holder::Ptr Open(const Parameters::Accessor& params, const Binary::Container& data)
  {
    using namespace ZXTune;
    for (PlayerPlugin::Iterator::Ptr usedPlugins = PlayerPluginsEnumerator::Create()->Enumerate(); usedPlugins->IsValid(); usedPlugins->Next())
    {
      const PlayerPlugin::Ptr plugin = usedPlugins->Get();
      if (const Holder::Ptr res = plugin->Open(params, data))
      {
        return res;
      }
    }
    throw Error(THIS_LINE, translate("Failed to find module at specified location."));
  }

  std::size_t Detect(const Parameters::Accessor& params, ZXTune::DataLocation::Ptr location, const DetectCallback& callback)
  {
    using namespace ZXTune;
    const ArchivePluginsEnumerator::Ptr usedArchivePlugins = ArchivePluginsEnumerator::Create();
    if (std::size_t usedSize = DetectByPlugins<ArchivePlugin>(params, usedArchivePlugins->Enumerate(), location, callback))
    {
      return usedSize;
    }
    return OpenInternal(params, location, callback);
  }

  std::size_t Detect(const Parameters::Accessor& params, Binary::Container::Ptr data, const DetectCallback& callback)
  {
    const ResolveAdditionalFilesAdapter adapter(params, data, callback);
    return Detect(params, ZXTune::CreateLocation(data), adapter);
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
