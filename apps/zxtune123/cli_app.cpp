/*
Abstract:
  CLI application implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
  
  This file is a part of zxtune123 application based on zxtune library
*/

//local includes
#include "console.h"
#include "information.h"
#include "sound.h"
#include "source.h"
#include <apps/base/app.h>
#include <apps/base/error_codes.h>
#include <apps/base/parsing.h>
//common includes
#include <error_tools.h>
#include <template.h>
//library includes
#include <core/convert_parameters.h>
#include <core/core_parameters.h>
#include <core/error_codes.h>
#include <core/module_attrs.h>
#include <core/plugin.h>
#include <core/plugin_attrs.h>
#include <io/fs_tools.h>
#include <io/providers_parameters.h>
#include <sound/sound_parameters.h>
//std includes
#include <algorithm>
#include <cctype>
#include <functional>
#include <fstream>
#include <iostream>
#include <limits>
#include <numeric>
//boost includes
#include <boost/bind.hpp>
#include <boost/program_options.hpp>
#include <boost/thread/thread.hpp>
//text includes
#include "text/text.h"

#define FILE_TAG 81C76E7D

// version definition-related
#ifndef ZXTUNE_VERSION
#define ZXTUNE_VERSION develop
#endif

#define STR(a) #a

#define VERSION_STRING(a) STR(a)

namespace
{
  //layout constants
  //TODO: make dynamic calculation
  const std::size_t INFORMATION_HEIGHT = 6;
  const std::size_t TRACKING_HEIGHT = 4;
  const std::size_t PLAYING_HEIGHT = 2;
  
  inline void ErrOuter(uint_t /*level*/, Error::LocationRef loc, Error::CodeType code, const String& text)
  {
    StdOut << Error::AttributesToString(loc, code, text);
  }

  inline void OutProp(const StringMap::value_type& prop)
  {
    StdOut << prop.first << '=' << prop.second << std::endl;
  }

  inline void ShowItemInfo(const ZXTune::Module::Information& info, uint_t frameDuration)
  {
    StringMap strProps;
    Parameters::ConvertMap(info.Properties, strProps);
#if 1
    StdOut
      << std::endl
      << InstantiateTemplate(Text::ITEM_INFO, strProps, FILL_NONEXISTING)
      << (Formatter(Text::ITEM_INFO_ADDON) % FormatTime(info.Statistic.Frame, frameDuration) %
        info.Statistic.Channels % info.PhysicalChannels).str();
#else
    std::for_each(strProps.begin(), strProps.end(), OutProp);
#endif
  }

  inline void ShowTrackingStatus(const ZXTune::Module::Tracking& track)
  {
    const String& dump = (Formatter(Text::TRACKING_STATUS)
      % track.Position % track.Pattern % track.Line % track.Frame % track.Tempo % track.Channels).str();
    assert(TRACKING_HEIGHT == static_cast<std::size_t>(std::count(dump.begin(), dump.end(), '\n')));
    StdOut << dump;
  }
  
  inline Char StateSymbol(ZXTune::Sound::Backend::State state)
  {
    switch (state)
    {
    case ZXTune::Sound::Backend::STARTED:
      return '>';
    case ZXTune::Sound::Backend::PAUSED:
      return '#';
    default:
      return '\?';
    }
  }
  
  void ShowPlaybackStatus(uint_t frame, uint_t allframe, ZXTune::Sound::Backend::State state, uint_t width,
                          uint_t frameDuration)
  {
    const Char MARKER = '\x1';
    String data((Formatter(Text::PLAYBACK_STATUS) % FormatTime(frame, frameDuration) % MARKER).str());
    const String::size_type totalSize = data.size() - 1 - PLAYING_HEIGHT;
    const String::size_type markerPos = data.find(MARKER);
    
    String prog(width - totalSize, '-');
    const uint_t pos = frame * (width - totalSize) / allframe;
    prog[pos] = StateSymbol(state);
    data.replace(markerPos, 1, prog);
    assert(PLAYING_HEIGHT == static_cast<std::size_t>(std::count(data.begin(), data.end(), '\n')));
    StdOut << data << std::flush;
  }
  
  void UpdateAnalyzer(const ZXTune::Module::Analyze::ChannelsState& inState,
    uint_t fallspeed, std::vector<int_t>& outState)
  {
    std::transform(outState.begin(), outState.end(), outState.begin(),
      std::bind2nd(std::minus<int_t>(), fallspeed));
    for (uint_t chan = 0, lim = inState.size(); chan != lim; ++chan)
    {
      const ZXTune::Module::Analyze::Channel& state(inState[chan]);
      if (state.Enabled && state.Band < outState.size())
      {
        outState[state.Band] = state.Level;
      }
    }
    std::replace_if(outState.begin(), outState.end(), std::bind2nd(std::less<int_t>(), 0), 0);
  }
  
  inline char SymIfGreater(int_t val, int_t limit)
  {
    return val > limit ? '#' : ' ';
  }
  
  void ShowAnalyzer(const std::vector<int_t>& state, uint_t high)
  {
    const uint_t width = state.size();
    std::string buffer(width, ' ');
    for (int_t y = high; y; --y)
    {
      const int_t limit = (y - 1) * std::numeric_limits<ZXTune::Module::Analyze::LevelType>::max() / high;
      std::transform(state.begin(), state.end(), buffer.begin(), boost::bind(SymIfGreater, _1, limit));
      StdOut << buffer << '\n';
    }
    StdOut << std::flush;
  }
  
  class Convertor
  {
  public:
    Convertor(const String& paramsStr, bool silent)
      : Silent(silent)
    {
      assert(!paramsStr.empty());
      Parameters::Map paramsMap;
      ThrowIfError(ParseParametersString(String(), paramsStr, paramsMap));
      Parameters::StringType mode;
      if (!Parameters::FindByName(paramsMap, Text::CONVERSION_PARAM_MODE, mode))
      {
        throw Error(THIS_LINE, CONVERT_PARAMETERS, Text::CONVERT_ERROR_NO_MODE);
      }
      if (!Parameters::FindByName(paramsMap, Text::CONVERSION_PARAM_FILENAME, NameTemplate))
      {
        throw Error(THIS_LINE, CONVERT_PARAMETERS, Text::CONVERT_ERROR_NO_FILENAME);
      }
      if (mode == Text::CONVERSION_MODE_RAW)
      {
        ConversionParameter.reset(new ZXTune::Module::Conversion::RawConvertParam());
        CapabilityMask = ZXTune::CAP_CONV_RAW;
      }
      else if (mode == Text::CONVERSION_MODE_PSG)
      {
        ConversionParameter.reset(new ZXTune::Module::Conversion::PSGConvertParam());
        CapabilityMask = ZXTune::CAP_CONV_PSG;
      }
      else if (mode == Text::CONVERSION_MODE_ZX50)
      {
        ConversionParameter.reset(new ZXTune::Module::Conversion::ZX50ConvertParam());
        CapabilityMask = ZXTune::CAP_CONV_ZX50;
      }
      else if (mode == Text::CONVERSION_MODE_TXT)
      {
        ConversionParameter.reset(new ZXTune::Module::Conversion::TXTConvertParam());
        CapabilityMask = ZXTune::CAP_CONV_TXT;
      }
      else
      {
        throw Error(THIS_LINE, CONVERT_PARAMETERS, Text::CONVERT_ERROR_INVALID_MODE);
      }
    }
    bool ProcessItem(const ModuleItem& item) const
    {
      {
        ZXTune::PluginInformation info;
        item.Module->GetPluginInformation(info);
        if (!(info.Capabilities & CapabilityMask))
        {
          Message(Text::CONVERT_SKIPPED, item.Id, info.Id);
          return true;
        }
      }
      Dump result;
      ThrowIfError(item.Module->Convert(*ConversionParameter, result));
      //prepare result filename
      ZXTune::Module::Information info;
      item.Module->GetModuleInformation(info);
      StringMap fields;
      {
        StringMap origFields;
        Parameters::ConvertMap(info.Properties, origFields);
        std::transform(origFields.begin(), origFields.end(), std::inserter(fields, fields.end()),
          boost::bind(&std::make_pair<String, String>,
            boost::bind<String>(&StringMap::value_type::first, _1),
            boost::bind<String>(&ZXTune::IO::MakePathFromString,
              boost::bind<String>(&StringMap::value_type::second, _1), '_')));
      }
      const String& filename = InstantiateTemplate(NameTemplate, fields, SKIP_NONEXISTING);
      std::ofstream file(filename.c_str(), std::ios::binary);
      file.write(safe_ptr_cast<const char*>(&result[0]), static_cast<std::streamsize>(result.size() * sizeof(result.front())));
      if (!file)
      {
        throw MakeFormattedError(THIS_LINE, CONVERT_PARAMETERS,
          Text::CONVERT_ERROR_WRITE_FILE, filename);
      }
      Message(Text::CONVERT_DONE, item.Id, filename);
      return true;
    }
  private:
    template<class P1, class P2>
    void Message(const String& format, const P1& p1, const P2& p2) const
    {
      if (!Silent)
      {
        StdOut << (Formatter(format) % p1 % p2).str() << std::endl;
      }
    }

    template<class P1, class P2, class P3>
    void Message(const String& format, const P1& p1, const P2& p2, const P3& p3) const
    {
      if (!Silent)
      {
        StdOut << (Formatter(format) % p1 % p2 % p3).str() << std::endl;
      }
    }
  private:
    const bool Silent;
    std::auto_ptr<ZXTune::Module::Conversion::Parameter> ConversionParameter;
    uint_t CapabilityMask;
    String NameTemplate;
  };
  
  class CLIApplication : public Application
  {
  public:
    CLIApplication()
      : Informer(InformationComponent::Create())
      , Sourcer(SourceComponent::Create(GlobalParams))
      , Sounder(SoundComponent::Create(GlobalParams))
      , Silent(false)
      , Quiet(false)
      , Analyzer(false)
      , SeekStep(10)
      , Updatefps(10)
    {
    }
    
    virtual int Run(int argc, char* argv[])
    {
      try
      {
        if (ProcessOptions(argc, argv) ||
            Informer->Process())
        {
          return 0;
        }
        
        Sourcer->Initialize();
        Sounder->Initialize();
        
        if (!ConvertParams.empty())
        {
          Convertor cnv(ConvertParams, Silent);
          Sourcer->ProcessItems(boost::bind(&Convertor::ProcessItem, &cnv, _1));
        }
        else
        {
          Sourcer->ProcessItems(boost::bind(&CLIApplication::PlayItem, this, _1));
        }
      }
      catch (const Error& e)
      {
        if (!e.FindSuberror(ZXTune::Module::ERROR_DETECT_CANCELED))
        {
          e.WalkSuberrors(ErrOuter);
        }
        return -1;
      }
      return 0;
    }
  private:
    bool ProcessOptions(int argc, char* argv[])
    {
      try
      {
        using namespace boost::program_options;

        String configFile;
        String providersOptions, coreOptions;
        options_description options((Formatter(Text::USAGE_SECTION) % *argv).str());
        options.add_options()
          (Text::HELP_KEY, Text::HELP_DESC)
          (Text::VERSION_KEY, Text::VERSION_DESC)
          (Text::CONFIG_KEY, boost::program_options::value<String>(&configFile), Text::CONFIG_DESC)
          (Text::IO_PROVIDERS_OPTS_KEY, boost::program_options::value<String>(&providersOptions), Text::IO_PROVIDERS_OPTS_DESC)
          (Text::CORE_OPTS_KEY, boost::program_options::value<String>(&coreOptions), Text::CORE_OPTS_DESC)
          (Text::CONVERT_KEY, boost::program_options::value<String>(&ConvertParams), Text::CONVERT_DESC)
        ;
        
        options.add(Informer->GetOptionsDescription());
        options.add(Sourcer->GetOptionsDescription());
        options.add(Sounder->GetOptionsDescription());
        //add positional parameters for input
        positional_options_description inputPositional;
        inputPositional.add(Text::INPUT_FILE_KEY, -1);
        
        //cli options
        options_description cliOptions(Text::CLI_SECTION);
        cliOptions.add_options()
          (Text::SILENT_KEY, bool_switch(&Silent), Text::SILENT_DESC)
          (Text::QUIET_KEY, bool_switch(&Quiet), Text::QUIET_DESC)
          (Text::ANALYZER_KEY, bool_switch(&Analyzer), Text::ANALYZER_DESC)
          (Text::SEEKSTEP_KEY, value<uint_t>(&SeekStep), Text::SEEKSTEP_DESC)
          (Text::UPDATEFPS_KEY, value<uint_t>(&Updatefps), Text::UPDATEFPS_DESC)
        ;
        options.add(cliOptions);
        
        variables_map vars;
        store(command_line_parser(argc, argv).options(options).positional(inputPositional).run(), vars);
        notify(vars);
        if (vars.count(Text::HELP_KEY))
        {
          StdOut << options << std::endl;
          return true;
        }
        else if (vars.count(Text::VERSION_KEY))
        {
          StdOut << VERSION_STRING(ZXTUNE_VERSION) << std::endl;
          return true;
        }
        if (!providersOptions.empty())
        {
          Parameters::Map ioParams;
          ThrowIfError(ParseParametersString(Parameters::ZXTune::IO::Providers::PREFIX, providersOptions, ioParams));
          GlobalParams.insert(ioParams.begin(), ioParams.end());
        }
        if (!coreOptions.empty())
        {
          Parameters::Map coreParams;
          ThrowIfError(ParseParametersString(Parameters::ZXTune::Core::PREFIX, coreOptions, coreParams));
          GlobalParams.insert(coreParams.begin(), coreParams.end());
        }
        {
          Parameters::Map configParams;
          ThrowIfError(ParseConfigFile(configFile, configParams));
          Parameters::MergeMaps(GlobalParams, configParams, GlobalParams, false);
        }
        return false;
      }
      catch (const std::exception& e)
      {
        throw MakeFormattedError(THIS_LINE, UNKNOWN_ERROR, Text::COMMON_ERROR, e.what());
      }
    }
    
    bool PlayItem(const ModuleItem& item)
    {
      try
      {
        //calculate and apply parameters
        Parameters::Map perItemParams(GlobalParams);
        perItemParams.insert(item.Params.begin(), item.Params.end());
        ZXTune::Sound::Backend& backend(Sounder->GetBackend());
        ThrowIfError(backend.SetModule(item.Module));
        ThrowIfError(backend.SetParameters(perItemParams));

        Parameters::IntType frameDuration = 0;
        if (!Parameters::FindByName(perItemParams, Parameters::ZXTune::Sound::FRAMEDURATION, frameDuration))
        {
          frameDuration = Parameters::ZXTune::Sound::FRAMEDURATION_DEFAULT;
        }

        //show startup info
        if (!Silent)
        {
          ShowItemInfo(item.Information, static_cast<uint_t>(frameDuration));
        }
        const uint_t seekStepFrames(item.Information.Statistic.Frame * SeekStep / 100);
        const uint_t waitPeriod(std::max<uint_t>(1, 1000 / std::max<uint_t>(Updatefps, 1)));
        ThrowIfError(backend.Play());
        std::vector<int_t> analyzer;
        Console::SizeType scrSize;
        ZXTune::Module::Player::ConstWeakPtr weakPlayer(backend.GetPlayer());
        ZXTune::Sound::Gain curVolume = ZXTune::Sound::Gain();
        ZXTune::Sound::MultiGain allVolume;
        ZXTune::Sound::VolumeControl::Ptr volCtrl(backend.GetVolumeControl());
        const bool noVolume = volCtrl.get() == 0;
        if (!noVolume)
        {
          ThrowIfError(volCtrl->GetVolume(allVolume));
          curVolume = std::accumulate(allVolume.begin(), allVolume.end(), curVolume) / allVolume.size();
        }
        for (;;)
        {
          if (!Silent && !Quiet)
          {
            scrSize = Console::Self().GetSize();
            if (scrSize.first <= 0 || scrSize.second <= 0)
            {
              Silent = true;
            }
          }
          ZXTune::Sound::Backend::State state;
          ThrowIfError(backend.GetCurrentState(state));

          uint_t curFrame = 0;
          ZXTune::Module::Tracking curTracking;
          ZXTune::Module::Analyze::ChannelsState curAnalyze;
          ThrowIfError(weakPlayer.lock()->GetPlaybackState(curFrame, curTracking, curAnalyze));

          if (!Silent && !Quiet)
          {
            const int_t spectrumHeight = scrSize.second - INFORMATION_HEIGHT - TRACKING_HEIGHT - PLAYING_HEIGHT - 1;
            if (spectrumHeight < 4)//minimal spectrum height
            {
              Analyzer = false;
            }
            else if (scrSize.second < int_t(TRACKING_HEIGHT + PLAYING_HEIGHT))
            {
              Quiet = true;
            }
            else
            {
              ShowTrackingStatus(curTracking);
              ShowPlaybackStatus(curFrame, item.Information.Statistic.Frame, state, scrSize.first, static_cast<uint_t>(frameDuration));
              if (Analyzer)
              {
                analyzer.resize(scrSize.first);
                UpdateAnalyzer(curAnalyze, 10, analyzer);
                ShowAnalyzer(analyzer, spectrumHeight);
              }
            }
          }
          if (const uint_t key = Console::Self().GetPressedKey())
          {
            switch (key)
            {
            case Console::INPUT_KEY_CANCEL:
            case 'Q':
              return false;
            case Console::INPUT_KEY_LEFT:
              ThrowIfError(backend.SetPosition(curFrame < seekStepFrames ? 0 : curFrame - seekStepFrames));
              break;
            case Console::INPUT_KEY_RIGHT:
              ThrowIfError(backend.SetPosition(curFrame + seekStepFrames));
              break;
            case Console::INPUT_KEY_DOWN:
              if (!noVolume)
              {
                curVolume = std::max(0.0, curVolume - 0.05);
                ZXTune::Sound::MultiGain allVol;
                allVol.assign(curVolume);
                ThrowIfError(volCtrl->SetVolume(allVol));
              }
              break;
            case Console::INPUT_KEY_UP:
              if (!noVolume)
              {
                curVolume = std::min(1.0, curVolume + 0.05);
                ZXTune::Sound::MultiGain allVol;
                allVol.assign(curVolume);
                ThrowIfError(volCtrl->SetVolume(allVol));
              }
              break;
            case Console::INPUT_KEY_ENTER:
              if (ZXTune::Sound::Backend::STARTED == state)
              {
                ThrowIfError(backend.Pause());
                Console::Self().WaitForKeyRelease();
              }
              else
              {
                Console::Self().WaitForKeyRelease();
                ThrowIfError(backend.Play());
              }
              break;
            case ' ':
              ThrowIfError(backend.Stop());
              state = ZXTune::Sound::Backend::STOPPED;
              Console::Self().WaitForKeyRelease();
              break;
            }
          }

          if (ZXTune::Sound::Backend::STOPPED == state)
          {
            break;
          }
          boost::this_thread::sleep(boost::posix_time::milliseconds(waitPeriod));
          if (!Silent && !Quiet)
          {
            Console::Self().MoveCursorUp(Analyzer ? scrSize.second - INFORMATION_HEIGHT - 1 : TRACKING_HEIGHT + PLAYING_HEIGHT);
          }
        }
      }
      catch (const Error& e)
      {
        e.WalkSuberrors(ErrOuter);
      }
      return true;
    }
  private:
    Parameters::Map GlobalParams;
    String ConvertParams;
    std::auto_ptr<InformationComponent> Informer;
    std::auto_ptr<SourceComponent> Sourcer;
    std::auto_ptr<SoundComponent> Sounder;
    bool Silent;
    bool Quiet;
    bool Analyzer;
    uint_t SeekStep;
    uint_t Updatefps;
  };
}

const std::string THIS_MODULE("zxtune123");

std::auto_ptr<Application> Application::Create()
{
  return std::auto_ptr<Application>(new CLIApplication());
}
