/**
 *
 * @file
 *
 * @brief Sound component implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "sound.h"
#include "config.h"
// common includes
#include <error_tools.h>
// library includes
#include <core/core_parameters.h>
#include <debug/log.h>
#include <math/numeric.h>
#include <parameters/merged_accessor.h>
#include <parameters/serialize.h>
#include <platform/application.h>
#include <sound/backends_parameters.h>
#include <sound/render_params.h>
#include <sound/service.h>
#include <sound/sound_parameters.h>
#include <strings/array.h>
#include <strings/map.h>
// std includes
#include <algorithm>
#include <cctype>
#include <iostream>
#include <list>
#include <sstream>
// boost includes
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/value_semantic.hpp>

namespace
{
  const Debug::Stream Dbg("zxtune123::Sound");

  static const String NOTUSED_MARK("\x01\x02");

  class CommonBackendParameters
  {
  public:
    CommonBackendParameters(Parameters::Container::Ptr config)
      : Params(std::move(config))
    {}

    void SetBackendParameters(const String& id, const String& options)
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
    typedef Strings::ValueMap<String> PerBackendOptions;

  public:
    explicit Component(Parameters::Container::Ptr configParams)
      : Service(Sound::CreateGlobalService(configParams))
      , Params(new CommonBackendParameters(configParams))
      , OptionsDescription("Sound options")
      , Looped(false)
    {
      using namespace boost::program_options;
      auto opt = OptionsDescription.add_options();
      for (auto backends = Service->EnumerateBackends(); backends->IsValid(); backends->Next())
      {
        const auto info = backends->Get();
        if (info->Status())
        {
          continue;
        }
        const auto id = info->Id().to_string();
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

    Sound::Backend::Ptr CreateBackend(Module::Holder::Ptr module, const String& typeHint,
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
      for (auto backends = Service->EnumerateBackends(); backends->IsValid(); backends->Next())
      {
        const auto info = backends->Get();
        const auto id = info->Id();
        if (BackendOptions.empty() || BackendOptions.count(id))
        {
          Dbg("Trying backend {}", id);
          try
          {
            const Sound::Backend::Ptr result = Service->CreateBackend(id, module, callback);
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

    Sound::BackendInformation::Iterator::Ptr EnumerateBackends() const override
    {
      return Service->EnumerateBackends();
    }

    uint_t GetSamplerate() const
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

    bool Looped;

    String UsedId;
  };
}  // namespace

std::unique_ptr<SoundComponent> SoundComponent::Create(Parameters::Container::Ptr configParams)
{
  return std::unique_ptr<SoundComponent>(new Component(configParams));
}
