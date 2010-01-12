#include "error_codes.h"
#include "parsing.h"
#include "sound.h"

#include <error_tools.h>
#include <logging.h>
#include <core/core_parameters.h>
#include <sound/backends_parameters.h>
#include <sound/render_params.h>
#include <sound/sound_parameters.h>

#include <algorithm>
#include <boost/bind.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/value_semantic.hpp>

#include "cmdline.h"

#define FILE_TAG DAEDAE2A

namespace
{
  static const String NOTUSED_MARK("\x01\x02");
  
  class Sound : public SoundComponent
  {
    typedef std::list<std::pair<String, String> > PerBackendOptions;
  public:
    explicit Sound(Parameters::Map& globalParams)
      : GlobalParams(globalParams)
      , OptionsDescription(TEXT_SOUND_SECTION)
      , YM(false)
      , Looped(false)
    {
      using namespace boost::program_options;
      ZXTune::Sound::BackendInfoArray backends;
      ZXTune::Sound::EnumerateBackends(backends);
      for (ZXTune::Sound::BackendInfoArray::const_iterator it = backends.begin(), lim = backends.end(); it != lim; ++it)
      {
        BackendOptions.push_back(std::make_pair(it->Id, NOTUSED_MARK));
        OptionsDescription.add_options()
          (it->Id.c_str(), value<String>(&BackendOptions.back().second)->implicit_value(String(),
            TEXT_SOUND_BACKEND_PARAMS), it->Description.c_str())
          ;
      }

      OptionsDescription.add_options()
        (TEXT_FREQUENCY_KEY, value<String>(&SoundOptions[Parameters::ZXTune::Sound::FREQUENCY]), TEXT_FREQUENCY_DESC)
        (TEXT_CLOCKRATE_KEY, value<String>(&SoundOptions[Parameters::ZXTune::Sound::CLOCKRATE]), TEXT_CLOCKRATE_DESC)
        (TEXT_FRAMEDURATION_KEY, value<String>(&SoundOptions[Parameters::ZXTune::Sound::FRAMEDURATION]), TEXT_FRAMEDURATION_DESC)
        (TEXT_FREQTABLE_KEY, value<String>(&SoundOptions[Parameters::ZXTune::Core::AYM::TABLE]), TEXT_FREQTABLE_DESC)
        (TEXT_YM_KEY, bool_switch(&YM), TEXT_YM_DESC)
        (TEXT_LOOP_KEY, bool_switch(&Looped), TEXT_LOOP_DESC)
      ;
    }
    
    virtual const boost::program_options::options_description& GetOptionsDescription() const
    {
      return OptionsDescription;
    }
    
    // throw
    virtual void Initialize()
    {
      StringArray backends;
      {
        Parameters::Map params;
        for (PerBackendOptions::const_iterator it = BackendOptions.begin(), lim = BackendOptions.end(); it != lim; ++it)
        {
          if (it->second != NOTUSED_MARK)
          {
            backends.push_back(it->first);
            if (!it->second.empty())
            {
              Parameters::Map perBackend;
              ThrowIfError(ParseParametersString(String(Parameters::ZXTune::Sound::Backends::PREFIX) + it->first,
                it->second, perBackend));
              params.insert(perBackend.begin(), perBackend.end());
            }
          }
        }
        GlobalParams.insert(params.begin(), params.end());
      }
      {
        StringMap optimized;
        std::remove_copy_if(SoundOptions.begin(), SoundOptions.end(), std::inserter(optimized, optimized.end()),
          boost::bind(&String::empty, boost::bind(&StringMap::value_type::second, _1)));
        if (!optimized.empty())
        {
          Parameters::Map sndparams;
          Parameters::ConvertMap(optimized, sndparams);
          GlobalParams.insert(sndparams.begin(), sndparams.end());
        }
        if (YM)
        {
          GlobalParams.insert(Parameters::Map::value_type(Parameters::ZXTune::Core::AYM::TYPE, -1));
        }
        if (Looped)
        {
          GlobalParams.insert(Parameters::Map::value_type(Parameters::ZXTune::Sound::LOOPMODE, ZXTune::Sound::LOOP_NORMAL));
        }
      }
      if (backends.empty())
      {
        backends.resize(BackendOptions.size());
        std::transform(BackendOptions.begin(), BackendOptions.end(), backends.begin(),
          boost::mem_fn(&PerBackendOptions::value_type::first));
      }
      
      for (StringArray::const_iterator it = backends.begin(), lim = backends.end(); it != lim; ++it)
      {
        ZXTune::Sound::Backend::Ptr backend;
        Log::Debug(THIS_MODULE, "Trying backend %1%", *it);
        if (/*const Error& e = */ZXTune::Sound::CreateBackend(*it, backend))
        {
          Log::Debug(THIS_MODULE, " failed");
          continue;
        }
        Log::Debug(THIS_MODULE, "Success!");
        Backend = backend;
        return;
      }
      throw Error(THIS_LINE, NO_BACKENDS, "Failed to create backend.");
    }

    virtual ZXTune::Sound::Backend& GetBackend()
    {
      return *Backend;
    }
    
  private:
    Parameters::Map& GlobalParams;
    boost::program_options::options_description OptionsDescription;
    PerBackendOptions BackendOptions;
    StringMap SoundOptions;
    ZXTune::Sound::Backend::Ptr Backend;
    bool YM;
    bool Looped;
  };
}

std::auto_ptr<SoundComponent> SoundComponent::Create(Parameters::Map& globalParams)
{
  return std::auto_ptr<SoundComponent>(new Sound(globalParams));
}
