/*
Abstract:
  Sound component implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
  
  This file is a part of zxtune123 application based on zxtune library
*/

#include "app.h"
#include "error_codes.h"
#include "parsing.h"
#include "sound.h"

#include <error_tools.h>
#include <logging.h>
#include <string_helpers.h>
#include <core/core_parameters.h>
#include <sound/backends_parameters.h>
#include <sound/filter.h>
#include <sound/render_params.h>

#include <algorithm>
#include <cctype>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/bind.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/value_semantic.hpp>

#include <iostream>

#include "text.h"

#define FILE_TAG DAEDAE2A

namespace
{
  const Char DEFAULT_MIXER_3[] = {'A', 'B', 'C', '\0'};
  const Char DEFAULT_MIXER_4[] = {'A', 'B', 'C', 'D', '\0'};

  static const String NOTUSED_MARK("\x01\x02");

  const unsigned MIN_CHANNELS = 2;
  const unsigned MAX_CHANNELS = 6;
  const unsigned DEFAULT_FILTER_ORDER = 10;

  const Char MATRIX_DELIMITERS[] = {';', ',', '-', '\0'};

  void ErrOuter(unsigned /*level*/, Error::LocationRef loc, Error::CodeType code, const String& text)
  {
    StdOut << Error::AttributesToString(loc, code, text);
  }
  
  inline bool InvalidChannelLetter(unsigned channels, Char letter)
  {
    if (channels < MIN_CHANNELS || channels > MAX_CHANNELS)
    {
      return true;
    }
    return (letter < Char('A') || letter > Char('A' + channels)) &&
           (letter < Char('a') || letter > Char('a' + channels)) &&
           letter != Char('-');
  }

  template<class T>
  inline T FromString(const String& str)
  {
    InStringStream stream(str);
    T res = 0;
    if (stream >> res)
    {
      return res;
    }
    throw MakeFormattedError(THIS_LINE, INVALID_PARAMETER, TEXT_ERROR_INVALID_FORMAT, str);
  }

  //ABC: 1.0,0.0,0.5,0.5,0.0,1.0
  std::vector<ZXTune::Sound::MultiGain> ParseMixerMatrix(const String& str)
  {
    //check for layout
    if (str.end() == std::find_if(str.begin(), str.end(), std::bind1st(std::ptr_fun(&InvalidChannelLetter), static_cast<unsigned>(str.size()))))
    {
      const unsigned channels = static_cast<unsigned>(str.size());
      //letter- position in result matrix
      //letter position- output channel
      //letter case- level
      std::vector<ZXTune::Sound::MultiGain> result(channels);
      for (unsigned inChannel = 0; inChannel != channels; ++inChannel)
      {
        if (str[inChannel] == '-')
        {
          continue;
        }
        const double inPos(1.0 * inChannel / (channels - 1));
        const bool gained(str[inChannel] < 'a');
        ZXTune::Sound::MultiGain& res(result[toupper(str[inChannel]) - 'A']);
        for (unsigned outChannel = 0; outChannel != res.size(); ++outChannel)
        {
          const double outPos(1.0 * outChannel / (res.size() - 1));
          res[outChannel] += (gained ? 1.0 : 0.6) * (1 - abs(inPos - outPos));
        }
      }
      const double maxGain(*std::max_element(result.front().begin(), result.back().end()));
      if (maxGain > 0)
      {
        std::transform(result.front().begin(), result.back().end(), result.front().begin(), std::bind2nd(std::divides<double>(), maxGain));
      }
      return result;
    }
    StringArray splitted;
    boost::algorithm::split(splitted, str, boost::algorithm::is_any_of(MATRIX_DELIMITERS));
    if (splitted.size() < MAX_CHANNELS * ZXTune::Sound::OUTPUT_CHANNELS ||
        splitted.size() > MIN_CHANNELS * ZXTune::Sound::OUTPUT_CHANNELS)
    {
      std::vector<ZXTune::Sound::MultiGain> result(splitted.size() / ZXTune::Sound::OUTPUT_CHANNELS);
      std::transform(splitted.begin(), splitted.end(), result.begin()->begin(), &FromString<double>);
      return result;
    }
    throw MakeFormattedError(THIS_LINE, INVALID_PARAMETER, TEXT_SOUND_ERROR_INVALID_MIXER, str);
  }

  ZXTune::Sound::Converter::Ptr CreateFilter(unsigned freq, const String& str)
  {
    //[order,]low-hi
    const Char PARAM_DELIMITER(',');
    const Char BANDPASS_DELIMITER('-');
    if (1 == std::count(str.begin(), str.end(), BANDPASS_DELIMITER) &&
        1 >= std::count(str.begin(), str.end(), PARAM_DELIMITER))
    {
      const String::size_type paramDPos(str.find(PARAM_DELIMITER));
      const String::size_type rangeDPos(str.find(BANDPASS_DELIMITER));
      if (String::npos == paramDPos || paramDPos < rangeDPos)
      {
        const unsigned order = String::npos == paramDPos ? DEFAULT_FILTER_ORDER : FromString<unsigned>(str.substr(0, paramDPos));
        const String::size_type rangeBegin(String::npos == paramDPos ? 0 : paramDPos + 1);
        const unsigned lowCutoff = FromString<unsigned>(str.substr(rangeBegin, rangeDPos - rangeBegin));
        const unsigned highCutoff = FromString<unsigned>(str.substr(rangeDPos + 1));
        ZXTune::Sound::Filter::Ptr filter;
        ThrowIfError(ZXTune::Sound::CreateFIRFilter(order, filter));
        ThrowIfError(filter->SetBandpassParameters(freq, lowCutoff, highCutoff));
        return filter;
      }
    }
    throw MakeFormattedError(THIS_LINE, INVALID_PARAMETER, TEXT_SOUND_ERROR_INVALID_FILTER, str);
  }

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
      ZXTune::Sound::BackendInformationArray backends;
      ZXTune::Sound::EnumerateBackends(backends);
      for (ZXTune::Sound::BackendInformationArray::const_iterator it = backends.begin(), lim = backends.end(); it != lim; ++it)
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
        (TEXT_MIXER_KEY, value<StringArray>(&Mixers), TEXT_MIXER_DESC)
        (TEXT_FILTER_KEY, value<String>(&Filter), TEXT_FILTER_DESC)
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
        Parameters::MergeMaps(GlobalParams, params, GlobalParams, true);
      }
      {
        StringMap optimized;
        std::remove_copy_if(SoundOptions.begin(), SoundOptions.end(), std::inserter(optimized, optimized.end()),
          boost::bind(&String::empty, boost::bind(&StringMap::value_type::second, _1)));
        if (!optimized.empty())
        {
          Parameters::Map sndparams;
          Parameters::ConvertMap(optimized, sndparams);
          Parameters::MergeMaps(GlobalParams, sndparams, GlobalParams, true);
        }
        if (YM)
        {
          GlobalParams[Parameters::ZXTune::Core::AYM::TYPE] = 1;
        }
        if (Looped)
        {
          GlobalParams[Parameters::ZXTune::Sound::LOOPMODE] = static_cast<Parameters::IntType>(ZXTune::Sound::LOOP_NORMAL);
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
        if (const Error& e = ZXTune::Sound::CreateBackend(*it, GlobalParams, backend))
        {
          Log::Debug(THIS_MODULE, " failed");
          if (1 == backends.size())
          {
            throw e;
          }
          e.WalkSuberrors(ErrOuter);
          continue;
        }
        Log::Debug(THIS_MODULE, "Success!");
        Backend = backend;
        break;
      }
      if (!Backend.get())
      {
        throw Error(THIS_LINE, NO_BACKENDS, TEXT_SOUND_ERROR_NO_BACKEND);
      }

      //setup mixers
      if (Mixers.empty())
      {
        Mixers.push_back(DEFAULT_MIXER_3);
        Mixers.push_back(DEFAULT_MIXER_4);
      }
      std::vector<std::vector<ZXTune::Sound::MultiGain> > matrixes(Mixers.size());
      std::transform(Mixers.begin(), Mixers.end(), matrixes.begin(), ParseMixerMatrix);
      std::for_each(matrixes.begin(), matrixes.end(),
        boost::bind(&ThrowIfError, boost::bind(&ZXTune::Sound::Backend::SetMixer, Backend.get(), _1)));
      //setup filter
      if (!Filter.empty())
      {
        Parameters::IntType freq = Parameters::ZXTune::Sound::FREQUENCY_DEFAULT;
        Parameters::FindByName(GlobalParams, Parameters::ZXTune::Sound::FREQUENCY, freq);
        ThrowIfError(Backend->SetFilter(CreateFilter(static_cast<unsigned>(freq), Filter)));
      }
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
    StringArray Mixers;
    String Filter;
  };
}

std::auto_ptr<SoundComponent> SoundComponent::Create(Parameters::Map& globalParams)
{
  return std::auto_ptr<SoundComponent>(new Sound(globalParams));
}
