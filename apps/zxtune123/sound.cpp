/**
* 
* @file
*
* @brief Sound component implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "config.h"
#include "sound.h"
//common includes
#include <error_tools.h>
//library includes
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
//std includes
#include <algorithm>
#include <cctype>
#include <list>
#include <iostream>
#include <sstream>
//boost includes
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/value_semantic.hpp>
//text includes
#include "text/text.h"

#define FILE_TAG DAEDAE2A

namespace
{
  const Debug::Stream Dbg("zxtune123::Sound");

  static const String NOTUSED_MARK("\x01\x02");

  template<class T>
  inline T FromString(const String& str)
  {
    std::basic_istringstream<Char> stream(str);
    T res = 0;
    if (stream >> res)
    {
      return res;
    }
    throw MakeFormattedError(THIS_LINE, Text::ERROR_INVALID_FORMAT, str);
  }

  class CommonBackendParameters
  {
  public:
    CommonBackendParameters(Parameters::Container::Ptr config)
      : Params(std::move(config))
    {
    }

    void SetBackendParameters(const String& id, const String& options)
    {
      ParseParametersString(Parameters::ZXTune::Sound::Backends::PREFIX + ToStdString(id),
        options, *Params);
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

    Time::Microseconds GetFrameDuration() const
    {
      return Sound::GetFrameDuration(*Params);
    }
  private:
    const Parameters::Container::Ptr Params;
  };

  class Component : public SoundComponent
  {
    //Id => Options
    typedef std::map<String, String> PerBackendOptions;
  public:
    explicit Component(Parameters::Container::Ptr configParams)
      : Service(Sound::CreateGlobalService(configParams))
      , Params(new CommonBackendParameters(configParams))
      , OptionsDescription(Text::SOUND_SECTION)
      , Looped(false)
    {
      using namespace boost::program_options;
      for (Sound::BackendInformation::Iterator::Ptr backends = Service->EnumerateBackends();
        backends->IsValid(); backends->Next())
      {
        const Sound::BackendInformation::Ptr info = backends->Get();
        if (info->Status())
        {
          continue;
        }
        const String id = info->Id();
        String& opts = BackendOptions[id];
        opts = NOTUSED_MARK;
        OptionsDescription.add_options()
          (id.c_str(), value<String>(&opts)->implicit_value(String(),
            Text::SOUND_BACKEND_PARAMS), info->Description().c_str())
          ;
      }

      OptionsDescription.add_options()
        (Text::FREQUENCY_KEY, value<String>(&SoundOptions[Parameters::ZXTune::Sound::FREQUENCY.FullPath()]), Text::FREQUENCY_DESC)
        (Text::FRAMEDURATION_KEY, value<String>(&SoundOptions[Parameters::ZXTune::Sound::FRAMEDURATION.FullPath()]), Text::FRAMEDURATION_DESC)
        (Text::FREQTABLE_KEY, value<String>(&SoundOptions[Parameters::ZXTune::Core::AYM::TABLE.FullPath()]), Text::FREQTABLE_DESC)
        (Text::LOOP_KEY, bool_switch(&Looped), Text::LOOP_DESC)
      ;
    }

    const boost::program_options::options_description& GetOptionsDescription() const override
    {
      return OptionsDescription;
    }

    void ParseParameters() override
    {
      Parameters::Container::Ptr soundParameters = Parameters::Container::Create();
      {
        for (auto it = BackendOptions.begin(), lim = BackendOptions.end(); it != lim; )
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
            const PerBackendOptions::iterator toRemove = it;
            ++it;
            BackendOptions.erase(toRemove);
          }
        }
      }
      Params->SetSoundParameters(SoundOptions);
      Params->SetLooped(Looped);
    }

    void Initialize() override
    {
    }

    Sound::Backend::Ptr CreateBackend(Module::Holder::Ptr module, const String& typeHint, Sound::BackendCallback::Ptr callback) override
    {
      if (!typeHint.empty())
      {
        return Service->CreateBackend(typeHint, module, callback);
      }
      if (!UsedId.empty())
      {
        Dbg("Using previously succeed backend %1%", UsedId);
        return Service->CreateBackend(UsedId, module, callback);
      }
      for (Sound::BackendInformation::Iterator::Ptr backends = Service->EnumerateBackends();
        backends->IsValid(); backends->Next())
      {
        const Sound::BackendInformation::Ptr info = backends->Get();
        const String id = info->Id();
        if (BackendOptions.empty() || BackendOptions.count(id))
        {
          Dbg("Trying backend %1%", id);
          try
          {
            const Sound::Backend::Ptr result = Service->CreateBackend(id, module, callback);
            Dbg("Success!");
            UsedId = id;
            return result;
          }
          catch (const Error& e)
          {
            Dbg(" failed:\n%1%", e.ToString());
            if (1 == BackendOptions.size())
            {
              throw;
            }
          }
        }
      }
      throw Error(THIS_LINE, Text::SOUND_ERROR_NO_BACKEND);
    }

    Time::Microseconds GetFrameDuration() const override
    {
      return Params->GetFrameDuration();
    }

    Sound::BackendInformation::Iterator::Ptr EnumerateBackends() const override
    {
      return Service->EnumerateBackends();
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
}

std::unique_ptr<SoundComponent> SoundComponent::Create(Parameters::Container::Ptr configParams)
{
  return std::unique_ptr<SoundComponent>(new Component(configParams));
}
