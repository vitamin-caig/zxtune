/*
Abstract:
  Sound component implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune123 application based on zxtune library
*/

//local includes
#include "sound.h"
#include <apps/base/app.h>
#include <apps/base/parsing.h>
//common includes
#include <error_tools.h>
#include <tools.h>
//library includes
#include <core/core_parameters.h>
#include <debug/log.h>
#include <math/numeric.h>
#include <sound/backends_parameters.h>
#include <sound/render_params.h>
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
#include <boost/bind.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/value_semantic.hpp>
//text includes
#include "text/text.h"
#include "../base/text/base_text.h"

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
      : Params(config)
    {
    }

    void SetBackendParameters(const String& id, const String& options)
    {
      ThrowIfError(ParseParametersString(Parameters::ZXTune::Sound::Backends::PREFIX + ToStdString(id),
        options, *Params));
    }

    void SetSoundParameters(const Strings::Map& options)
    {
      Strings::Map optimized;
      std::remove_copy_if(options.begin(), options.end(), std::inserter(optimized, optimized.end()),
        boost::bind(&String::empty, boost::bind(&Strings::Map::value_type::second, _1)));
      if (!optimized.empty())
      {
        Parameters::ParseStringMap(optimized, *Params);
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
      Parameters::IntType frameDuration = Parameters::ZXTune::Sound::FRAMEDURATION_DEFAULT;
      Params->FindValue(Parameters::ZXTune::Sound::FRAMEDURATION, frameDuration);
      return Time::Microseconds(static_cast<Time::Microseconds::ValueType>(frameDuration));
    }
  private:
    const Parameters::Container::Ptr Params;
  };

  class CreateBackendParams : public ZXTune::Sound::CreateBackendParameters
  {
  public:
    CreateBackendParams(const CommonBackendParameters& params, ZXTune::Module::Holder::Ptr module, ZXTune::Sound::BackendCallback::Ptr callback)
      : Params(params)
      , Module(module)
      , Callback(callback)
    {
    }

    virtual Parameters::Accessor::Ptr GetParameters() const
    {
      return Parameters::CreateMergedAccessor(Module->GetModuleProperties(), Params.GetDefaultParameters());
    }

    virtual ZXTune::Module::Holder::Ptr GetModule() const
    {
      return Module;
    }

    virtual ZXTune::Sound::BackendCallback::Ptr GetCallback() const
    {
      return Callback;
    }
  private:
    const CommonBackendParameters& Params;
    const ZXTune::Module::Holder::Ptr Module;
    const ZXTune::Sound::BackendCallback::Ptr Callback;
  };

  class Sound : public SoundComponent
  {
    typedef std::list<std::pair<ZXTune::Sound::BackendCreator::Ptr, String> > PerBackendOptions;
  public:
    explicit Sound(Parameters::Container::Ptr configParams)
      : Params(new CommonBackendParameters(configParams))
      , OptionsDescription(Text::SOUND_SECTION)
      , Looped(false)
    {
      using namespace boost::program_options;
      for (ZXTune::Sound::BackendCreator::Iterator::Ptr backends = ZXTune::Sound::EnumerateBackends();
        backends->IsValid(); backends->Next())
      {
        const ZXTune::Sound::BackendCreator::Ptr creator = backends->Get();
        if (creator->Status())
        {
          continue;
        }
        BackendOptions.push_back(std::make_pair(creator, NOTUSED_MARK));
        OptionsDescription.add_options()
          (creator->Id().c_str(), value<String>(&BackendOptions.back().second)->implicit_value(String(),
            Text::SOUND_BACKEND_PARAMS), creator->Description().c_str())
          ;
      }

      OptionsDescription.add_options()
        (Text::FREQUENCY_KEY, value<String>(&SoundOptions[Parameters::ZXTune::Sound::FREQUENCY.FullPath()]), Text::FREQUENCY_DESC)
        (Text::FRAMEDURATION_KEY, value<String>(&SoundOptions[Parameters::ZXTune::Sound::FRAMEDURATION.FullPath()]), Text::FRAMEDURATION_DESC)
        (Text::FREQTABLE_KEY, value<String>(&SoundOptions[Parameters::ZXTune::Core::AYM::TABLE.FullPath()]), Text::FREQTABLE_DESC)
        (Text::LOOP_KEY, bool_switch(&Looped), Text::LOOP_DESC)
      ;
    }

    virtual const boost::program_options::options_description& GetOptionsDescription() const
    {
      return OptionsDescription;
    }

    virtual void ParseParameters()
    {
      Parameters::Container::Ptr soundParameters = Parameters::Container::Create();
      {
        for (PerBackendOptions::const_iterator it = BackendOptions.begin(), lim = BackendOptions.end(); it != lim; ++it)
        {
          if (it->second != NOTUSED_MARK && !it->second.empty())
          {
            const ZXTune::Sound::BackendCreator::Ptr creator = it->first;
            Params->SetBackendParameters(creator->Id(), it->second);
          }
        }
      }
      Params->SetSoundParameters(SoundOptions);
      Params->SetLooped(Looped);
    }

    void Initialize()
    {
    }

    virtual ZXTune::Sound::Backend::Ptr CreateBackend(ZXTune::Module::Holder::Ptr module, const String& typeHint, ZXTune::Sound::BackendCallback::Ptr callback)
    {
      const ZXTune::Sound::CreateBackendParameters::Ptr createParams(new CreateBackendParams(*Params, module, callback));
      ZXTune::Sound::Backend::Ptr backend;
      if (!Creator)
      {
        std::list<ZXTune::Sound::BackendCreator::Ptr> backends;
        {
          for (PerBackendOptions::const_iterator it = BackendOptions.begin(), lim = BackendOptions.end(); it != lim; ++it)
          {
            const ZXTune::Sound::BackendCreator::Ptr creator = it->first;
            if (it->second != NOTUSED_MARK || creator->Id() == typeHint)
            {
              backends.push_back(creator);
            }
          }
        }
        if (backends.empty())
        {
          backends.resize(BackendOptions.size());
          std::transform(BackendOptions.begin(), BackendOptions.end(), backends.begin(),
            boost::mem_fn(&PerBackendOptions::value_type::first));
        }

        for (std::list<ZXTune::Sound::BackendCreator::Ptr>::const_iterator it =
          backends.begin(), lim = backends.end(); it != lim; ++it)
        {
          Dbg("Trying backend %1%", (*it)->Id());
          try
          {
            backend = (*it)->CreateBackend(createParams);
            Dbg("Success!");
            Creator = *it;
            break;
          }
          catch (const Error& e)
          {
            Dbg(" failed");
            if (1 == backends.size())
            {
              throw;
            }
            StdOut << e.ToString();
          }
        }
      }
      else
      {
        backend = Creator->CreateBackend(createParams);
      }
      if (!backend.get())
      {
        throw Error(THIS_LINE, Text::SOUND_ERROR_NO_BACKEND);
      }
      return backend;
    }

    virtual Time::Microseconds GetFrameDuration() const
    {
      return Params->GetFrameDuration();
    }
  private:
    const std::auto_ptr<CommonBackendParameters> Params;
    boost::program_options::options_description OptionsDescription;
    PerBackendOptions BackendOptions;
    Strings::Map SoundOptions;

    bool Looped;

    ZXTune::Sound::BackendCreator::Ptr Creator;
  };
}

std::auto_ptr<SoundComponent> SoundComponent::Create(Parameters::Container::Ptr configParams)
{
  return std::auto_ptr<SoundComponent>(new Sound(configParams));
}
