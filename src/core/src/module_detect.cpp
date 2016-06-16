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
#include "core/plugins/archive_plugins_enumerator.h"
#include "core/plugins/player_plugins_enumerator.h"
//common includes
#include <error.h>
#include <make_ptr.h>
//library includes
#include <debug/log.h>
#include <l10n/api.h>
#include <module/players/aym/aym_base.h>
#include <parameters/merged_accessor.h>
#include <parameters/container.h>
//text includes
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

    virtual void ProcessModule(ZXTune::DataLocation::Ptr /*location*/, ZXTune::Plugin::Ptr /*decoder*/, Module::Holder::Ptr holder) const
    {
      Result = holder;
    }

    virtual Log::ProgressCallback* GetProgress() const
    {
      return 0;
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

  //TODO: remove
  class MixedPropertiesHolder : public Holder
  {
  public:
    MixedPropertiesHolder(Holder::Ptr delegate, Parameters::Accessor::Ptr props)
      : Delegate(delegate)
      , Properties(props)
    {
    }

    virtual Information::Ptr GetModuleInformation() const
    {
      return Delegate->GetModuleInformation();
    }

    virtual Parameters::Accessor::Ptr GetModuleProperties() const
    {
      return Parameters::CreateMergedAccessor(Properties, Delegate->GetModuleProperties());
    }

    virtual Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target) const
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
      : Delegate(delegate)
      , Properties(props)
    {
    }

    virtual Information::Ptr GetModuleInformation() const
    {
      return Delegate->GetModuleInformation();
    }

    virtual Parameters::Accessor::Ptr GetModuleProperties() const
    {
      return Parameters::CreateMergedAccessor(Properties, Delegate->GetModuleProperties());
    }

    virtual Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target) const
    {
      return Delegate->CreateRenderer(Parameters::CreateMergedAccessor(params, Properties), target);
    }

    virtual Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Devices::AYM::Device::Ptr chip) const
    {
      return Delegate->CreateRenderer(Parameters::CreateMergedAccessor(params, Properties), chip);
    }

    virtual AYM::Chiptune::Ptr GetChiptune() const
    {
      return Delegate->GetChiptune();
    }
  private:
    const AYM::Holder::Ptr Delegate;
    const Parameters::Accessor::Ptr Properties;
  };

  void Open(const Parameters::Accessor& params, ZXTune::DataLocation::Ptr location, const DetectCallback& callback)
  {
    using namespace ZXTune;
    const PlayerPluginsEnumerator::Ptr usedPlayerPlugins = PlayerPluginsEnumerator::Create();
    if (!DetectByPlugins<PlayerPlugin>(params, usedPlayerPlugins->Enumerate(), location, callback))
    {
      throw Error(THIS_LINE, translate("Failed to find module at specified location."));
    }
  }

  Holder::Ptr Open(const Parameters::Accessor& params, ZXTune::DataLocation::Ptr location)
  {
    const OpenModuleCallback callback;
    Open(params, location, callback);
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
    const PlayerPluginsEnumerator::Ptr usedPlayerPlugins = PlayerPluginsEnumerator::Create();
    return DetectByPlugins<PlayerPlugin>(params, usedPlayerPlugins->Enumerate(), location, callback);
  }

  Holder::Ptr CreateMixedPropertiesHolder(Holder::Ptr delegate, Parameters::Accessor::Ptr props)
  {
    if (const AYM::Holder::Ptr aym = boost::dynamic_pointer_cast<const AYM::Holder>(delegate))
    {
      return MakePtr<MixedPropertiesAYMHolder>(aym, props);
    }
    return MakePtr<MixedPropertiesHolder>(delegate, props);
  }
}
