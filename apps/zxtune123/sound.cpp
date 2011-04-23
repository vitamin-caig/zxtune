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
#include <apps/base/error_codes.h>
#include <apps/base/parsing.h>
//common includes
#include <error_tools.h>
#include <logging.h>
#include <string_helpers.h>
#include <tools.h>
//library includes
#include <core/core_parameters.h>
#include <sound/backends_parameters.h>
#include <sound/filter.h>
#include <sound/render_params.h>
#include <sound/sound_parameters.h>
//std includes
#include <algorithm>
#include <cctype>
#include <list>
#include <iostream>
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
  const Char DEFAULT_MIXER_3[] = {'A', 'B', 'C', '\0'};
  const Char DEFAULT_MIXER_4[] = {'A', 'B', 'C', 'D', '\0'};

  static const String NOTUSED_MARK("\x01\x02");

  const uint_t MIN_CHANNELS = 2;
  const uint_t MAX_CHANNELS = 6;
  const uint_t DEFAULT_FILTER_ORDER = 10;

  const Char MATRIX_DELIMITERS[] = {';', ',', '-', '\0'};

  void ErrOuter(uint_t /*level*/, Error::LocationRef loc, Error::CodeType code, const String& text)
  {
    StdOut << Error::AttributesToString(loc, code, text);
  }

  inline bool InvalidChannelLetter(uint_t channels, Char letter)
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
    throw MakeFormattedError(THIS_LINE, INVALID_PARAMETER, Text::ERROR_INVALID_FORMAT, str);
  }

  //ABC: 1.0,0.0,0.5,0.5,0.0,1.0
  std::vector<ZXTune::Sound::MultiGain> ParseMixerMatrix(const String& str)
  {
    //check for layout
    if (str.end() == std::find_if(str.begin(), str.end(), std::bind1st(std::ptr_fun(&InvalidChannelLetter), static_cast<uint_t>(str.size()))))
    {
      const std::size_t channels = str.size();
      //letter- position in result matrix
      //letter position- output channel
      //letter case- level
      std::vector<ZXTune::Sound::MultiGain> result(channels);
      for (uint_t inChannel = 0; inChannel != channels; ++inChannel)
      {
        if (str[inChannel] == '-')
        {
          continue;
        }
        const double inPos(1.0 * inChannel / (channels - 1));
        const bool gained(str[inChannel] < 'a');
        ZXTune::Sound::MultiGain& res(result[toupper(str[inChannel]) - 'A']);
        for (uint_t outChannel = 0; outChannel != res.size(); ++outChannel)
        {
          const double outPos(1.0 * outChannel / (res.size() - 1));
          res[outChannel] += (gained ? 1.0 : 0.6) * (1.0 - absolute(inPos - outPos));
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
    if (splitted.size() <= MAX_CHANNELS * ZXTune::Sound::OUTPUT_CHANNELS ||
        splitted.size() >= MIN_CHANNELS * ZXTune::Sound::OUTPUT_CHANNELS)
    {
      std::vector<ZXTune::Sound::MultiGain> result(splitted.size() / ZXTune::Sound::OUTPUT_CHANNELS);
      std::transform(splitted.begin(), splitted.end(), result.begin()->begin(), &FromString<double>);
      return result;
    }
    throw MakeFormattedError(THIS_LINE, INVALID_PARAMETER, Text::SOUND_ERROR_INVALID_MIXER, str);
  }

  ZXTune::Sound::Converter::Ptr CreateFilter(uint_t freq, const String& str)
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
        const uint_t order = String::npos == paramDPos ? DEFAULT_FILTER_ORDER : FromString<unsigned>(str.substr(0, paramDPos));
        const String::size_type rangeBegin(String::npos == paramDPos ? 0 : paramDPos + 1);
        const uint_t lowCutoff = FromString<uint_t>(str.substr(rangeBegin, rangeDPos - rangeBegin));
        const uint_t highCutoff = FromString<uint_t>(str.substr(rangeDPos + 1));
        ZXTune::Sound::Filter::Ptr filter;
        ThrowIfError(ZXTune::Sound::CreateFIRFilter(order, filter));
        ThrowIfError(filter->SetBandpassParameters(freq, lowCutoff, highCutoff));
        return filter;
      }
    }
    throw MakeFormattedError(THIS_LINE, INVALID_PARAMETER, Text::SOUND_ERROR_INVALID_FILTER, str);
  }

  class BackendParams : public ZXTune::Sound::BackendParameters
  {
  public:
    BackendParams(Parameters::Container::Ptr config, const StringArray& mixers, const String& filter)
      : Params(config)
    {
      std::for_each(mixers.begin(), mixers.end(), boost::bind(&BackendParams::AddMixer, this, _1));
      if (!filter.empty())
      {
        Parameters::IntType freq = Parameters::ZXTune::Sound::FREQUENCY_DEFAULT;
        Params->FindIntValue(Parameters::ZXTune::Sound::FREQUENCY, freq);
        Filter = CreateFilter(static_cast<uint_t>(freq), filter);
      }
    }

    virtual Parameters::Accessor::Ptr GetDefaultParameters() const
    {
      return Params;
    }

    virtual ZXTune::Sound::Mixer::Ptr GetMixer(uint_t channels) const
    {
      ZXTune::Sound::Mixer::Ptr& mixer = Mixers[channels];
      if (!mixer)
      {
        ThrowIfError(ZXTune::Sound::CreateMixer(channels, mixer));
      }
      return mixer;
    }

    virtual ZXTune::Sound::Converter::Ptr GetFilter() const
    {
      return Filter;
    }
  private:
    void AddMixer(const String& txt)
    {
      const std::vector<ZXTune::Sound::MultiGain>& matrix = ParseMixerMatrix(txt);
      const uint_t chans = matrix.size();
      ZXTune::Sound::Mixer::Ptr& newMixer = Mixers[chans];
      ThrowIfError(ZXTune::Sound::CreateMixer(chans, newMixer));
      ThrowIfError(newMixer->SetMatrix(matrix));
    }
  private:
    const Parameters::Container::Ptr Params;
    mutable std::map<uint_t, ZXTune::Sound::Mixer::Ptr> Mixers;
    ZXTune::Sound::Converter::Ptr Filter;
  };

  class Sound : public SoundComponent
  {
    typedef std::list<std::pair<ZXTune::Sound::BackendCreator::Ptr, String> > PerBackendOptions;
  public:
    explicit Sound(Parameters::Container::Ptr configParams)
      : Params(configParams)
      , OptionsDescription(Text::SOUND_SECTION)
      , Looped(false)
    {
      using namespace boost::program_options;
      for (ZXTune::Sound::BackendCreator::Iterator::Ptr backends = ZXTune::Sound::EnumerateBackends();
        backends->IsValid(); backends->Next())
      {
        const ZXTune::Sound::BackendCreator::Ptr creator = backends->Get();
        BackendOptions.push_back(std::make_pair(creator, NOTUSED_MARK));
        OptionsDescription.add_options()
          (creator->Id().c_str(), value<String>(&BackendOptions.back().second)->implicit_value(String(),
            Text::SOUND_BACKEND_PARAMS), creator->Description().c_str())
          ;
      }

      OptionsDescription.add_options()
        (Text::FREQUENCY_KEY, value<String>(&SoundOptions[Parameters::ZXTune::Sound::FREQUENCY]), Text::FREQUENCY_DESC)
        (Text::CLOCKRATE_KEY, value<String>(&SoundOptions[Parameters::ZXTune::Sound::CLOCKRATE]), Text::CLOCKRATE_DESC)
        (Text::FRAMEDURATION_KEY, value<String>(&SoundOptions[Parameters::ZXTune::Sound::FRAMEDURATION]), Text::FRAMEDURATION_DESC)
        (Text::FREQTABLE_KEY, value<String>(&SoundOptions[Parameters::ZXTune::Core::AYM::TABLE]), Text::FREQTABLE_DESC)
        (Text::LOOP_KEY, bool_switch(&Looped), Text::LOOP_DESC)
        (Text::MIXER_KEY, value<StringArray>(&Mixers), Text::MIXER_DESC)
        (Text::FILTER_KEY, value<String>(&Filter), Text::FILTER_DESC)
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
            ThrowIfError(ParseParametersString(String(Parameters::ZXTune::Sound::Backends::PREFIX) + creator->Id(),
              it->second, *Params));
          }
        }
      }
      {
        StringMap optimized;
        std::remove_copy_if(SoundOptions.begin(), SoundOptions.end(), std::inserter(optimized, optimized.end()),
          boost::bind(&String::empty, boost::bind(&StringMap::value_type::second, _1)));
        if (!optimized.empty())
        {
          Parameters::ParseStringMap(optimized, *Params);
        }
        if (Looped)
        {
          Params->SetIntValue(Parameters::ZXTune::Sound::LOOPMODE, ZXTune::Sound::LOOP_NORMAL);
        }
      }
    }

    void Initialize()
    {
      //setup mixers
      if (Mixers.empty())
      {
        Mixers.push_back(DEFAULT_MIXER_3);
        Mixers.push_back(DEFAULT_MIXER_4);
      }
    }

    virtual ZXTune::Sound::Backend::Ptr CreateBackend(ZXTune::Module::Holder::Ptr module)
    {
      ZXTune::Sound::Backend::Ptr backend;
      if (!Creator || Creator->CreateBackend(GetParams(), module, backend))
      {
        std::list<ZXTune::Sound::BackendCreator::Ptr> backends;
        {
          for (PerBackendOptions::const_iterator it = BackendOptions.begin(), lim = BackendOptions.end(); it != lim; ++it)
          {
            if (it->second != NOTUSED_MARK)
            {
              const ZXTune::Sound::BackendCreator::Ptr creator = it->first;
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
          Log::Debug(THIS_MODULE, "Trying backend %1%", (*it)->Id());
          if (const Error& e = (*it)->CreateBackend(GetParams(), module, backend))
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
          Creator = *it;
          break;
        }
      }
      if (!backend.get())
      {
        throw Error(THIS_LINE, NO_BACKENDS, Text::SOUND_ERROR_NO_BACKEND);
      }
      return backend;
    }

    virtual uint_t GetFrameDuration() const
    {
      Parameters::IntType frameDuration = Parameters::ZXTune::Sound::FRAMEDURATION_DEFAULT;
      Params->FindIntValue(Parameters::ZXTune::Sound::FRAMEDURATION, frameDuration);
      return static_cast<uint_t>(frameDuration);
    }
  private:
    ZXTune::Sound::BackendParameters::Ptr GetParams() const
    {
      return ZXTune::Sound::BackendParameters::Ptr(new BackendParams(Params, Mixers, Filter));
    }
  private:
    const Parameters::Container::Ptr Params;
    boost::program_options::options_description OptionsDescription;
    PerBackendOptions BackendOptions;
    StringMap SoundOptions;

    bool Looped;
    StringArray Mixers;
    String Filter;

    ZXTune::Sound::BackendCreator::Ptr Creator;
  };
}

std::auto_ptr<SoundComponent> SoundComponent::Create(Parameters::Container::Ptr configParams)
{
  return std::auto_ptr<SoundComponent>(new Sound(configParams));
}
