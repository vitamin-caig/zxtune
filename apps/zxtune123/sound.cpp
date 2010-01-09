#include "error_codes.h"
#include "parsing.h"
#include "sound.h"

#include <error_tools.h>
#include <logging.h>
#include <sound/backends_parameters.h>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/value_semantic.hpp>

#include "messages.h"

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
    {
      ZXTune::Sound::BackendInfoArray backends;
      ZXTune::Sound::EnumerateBackends(backends);
      for (ZXTune::Sound::BackendInfoArray::const_iterator it = backends.begin(), lim = backends.end(); it != lim; ++it)
      {
        UnparsedOptions.push_back(std::make_pair(it->Id, NOTUSED_MARK));
        OptionsDescription.add(boost::shared_ptr<boost::program_options::option_description>(
          new boost::program_options::option_description(it->Id.c_str(), 
            boost::program_options::value<String>(&UnparsedOptions.back().second)->implicit_value(String(), TEXT_SOUND_BACKEND_PARAMS),
            it->Description.c_str())));
      }
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
        for (PerBackendOptions::const_iterator it = UnparsedOptions.begin(), lim = UnparsedOptions.end(); it != lim; ++it)
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
      if (backends.empty())
      {
        backends.resize(UnparsedOptions.size());
        std::transform(UnparsedOptions.begin(), UnparsedOptions.end(), backends.begin(),
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
    PerBackendOptions UnparsedOptions;
    ZXTune::Sound::Backend::Ptr Backend;
  };
}

std::auto_ptr<SoundComponent> SoundComponent::Create(Parameters::Map& globalParams)
{
  return std::auto_ptr<SoundComponent>(new Sound(globalParams));
}