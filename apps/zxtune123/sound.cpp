/**
 *
 * @file
 *
 * @brief Sound component implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune123/sound.h"

#include "apps/zxtune123/config.h"

#include "core/core_parameters.h"
#include "debug/log.h"
#include "parameters/identifier.h"
#include "parameters/serialize.h"
#include "sound/backend_attrs.h"
#include "sound/backends_parameters.h"
#include "sound/render_params.h"
#include "sound/service.h"
#include "sound/sound_parameters.h"
#include "strings/map.h"

#include "error.h"
#include "string_view.h"

#include <boost/program_options.hpp>

#include <utility>

namespace
{
  const Debug::Stream Dbg("zxtune123::Sound");

  const String NOTUSED_MARK("\x01\x02");

  class CommonBackendParameters
  {
  public:
    CommonBackendParameters(Parameters::Container::Ptr config)
      : Params(std::move(config))
    {}

    void SetBackendParameters(StringView id, StringView options)
    {
      using namespace Parameters;
      ParseParametersString(static_cast<Parameters::Identifier>(ZXTune::Sound::Backends::PREFIX).Append(id), options,
                            *Params);
    }

    void SetSoundParameters(const Strings::Map& options)
    {
      Strings::Map optimized;
      for (const auto& opt : options)
      {
        if (!opt.second.empty())
        {
          optimized.insert(opt);
        }
      }
      if (!optimized.empty())
      {
        Parameters::Convert(optimized, *Params);
      }
    }

    void SetLooped(bool looped)
    {
      if (looped)
      {
        Params->SetValue(Parameters::ZXTune::Sound::LOOPED, true);
      }
    }

    Parameters::Accessor::Ptr GetDefaultParameters() const
    {
      return Params;
    }

  private:
    const Parameters::Container::Ptr Params;
  };

  class Component : public SoundComponent
  {
    // Id => Options
    using PerBackendOptions = Strings::ValueMap<String>;

  public:
    explicit Component(Parameters::Container::Ptr configParams)
      : Service(Sound::CreateGlobalService(configParams))
      , Params(new CommonBackendParameters(std::move(configParams)))
      , OptionsDescription("Sound options")
    {
      using namespace boost::program_options;
      auto opt = OptionsDescription.add_options();
      for (const auto& info : Service->EnumerateBackends())
      {
        if (info->Status())
        {
          continue;
        }
        const auto id = String{info->Id()};
        String& opts = BackendOptions[id];
        opts = NOTUSED_MARK;
        opt(id.c_str(), value<String>(&opts)->implicit_value(String(), "parameters"), info->Description().c_str());
      }

      opt("frequency", value<String>(GetSoundOption(Parameters::ZXTune::Sound::FREQUENCY)),
          "specify sound frequency in Hz");
      opt("freqtable", value<String>(GetSoundOption(Parameters::ZXTune::Core::AYM::TABLE)), "specify frequency table");
      opt("loop", bool_switch(&Looped), "loop playback");
    }

    const boost::program_options::options_description& GetOptionsDescription() const override
    {
      return OptionsDescription;
    }

    void ParseParameters() override
    {
      {
        for (auto it = BackendOptions.begin(), lim = BackendOptions.end(); it != lim;)
        {
          if (it->second != NOTUSED_MARK)
          {
            if (!it->second.empty())
            {
              Params->SetBackendParameters(it->first, it->second);
            }
            ++it;
          }
          else
          {
            const auto toRemove = it;
            ++it;
            BackendOptions.erase(toRemove);
          }
        }
      }
      Params->SetSoundParameters(SoundOptions);
      Params->SetLooped(Looped);
    }

    void Initialize() override {}

    Sound::Backend::Ptr CreateBackend(Module::Holder::Ptr module, StringView typeHint,
                                      Sound::BackendCallback::Ptr callback) override
    {
      if (!typeHint.empty())
      {
        return Service->CreateBackend(Sound::BackendId::FromString(typeHint), module, callback);
      }
      if (!UsedId.empty())
      {
        Dbg("Using previously succeed backend {}", UsedId);
        return Service->CreateBackend(Sound::BackendId::FromString(UsedId), module, callback);
      }
      for (const auto& info : Service->EnumerateBackends())
      {
        const auto id = info->Id();
        if (BackendOptions.empty() || BackendOptions.count(id))
        {
          Dbg("Trying backend {}", id);
          try
          {
            auto result = Service->CreateBackend(id, module, callback);
            Dbg("Success!");
            UsedId = id;
            return result;
          }
          catch (const Error& e)
          {
            Dbg(" failed:\n{}", e.ToString());
            if (1 == BackendOptions.size())
            {
              throw;
            }
          }
        }
      }
      throw Error(THIS_LINE, "Failed to create any backend.");
    }

    std::span<const Sound::BackendInformation::Ptr> EnumerateBackends() const override
    {
      return Service->EnumerateBackends();
    }

    uint_t GetSamplerate() const override
    {
      return Sound::GetSoundFrequency(*Params->GetDefaultParameters());
    }

  private:
    String* GetSoundOption(Parameters::Identifier name)
    {
      return &SoundOptions[name.AsString()];
    }

  private:
    const Sound::Service::Ptr Service;
    const std::unique_ptr<CommonBackendParameters> Params;
    boost::program_options::options_description OptionsDescription;
    PerBackendOptions BackendOptions;
    Strings::Map SoundOptions;

    bool Looped = false;

    String UsedId;
  };
}  // namespace

std::unique_ptr<SoundComponent> SoundComponent::Create(Parameters::Container::Ptr configParams)
{
  return std::unique_ptr<SoundComponent>(new Component(std::move(configParams)));
}
