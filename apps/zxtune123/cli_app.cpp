/*
Abstract:
  CLI application implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
  
  This file is a part of zxtune123 application based on zxtune library
*/

#include "app.h"
#include "console.h"
#include "error_codes.h"
#include "information.h"
#include "parsing.h"
#include "sound.h"
#include "source.h"

#include <error_tools.h>
#include <template.h>
#include <core/convert_parameters.h>
#include <core/core_parameters.h>
#include <core/module_attrs.h>
#include <core/plugin.h>
#include <core/plugin_attrs.h>
#include <io/fs_tools.h>
#include <io/providers_parameters.h>
#include <sound/sound_parameters.h>

#include <boost/bind.hpp>
#include <boost/program_options.hpp>

#include <algorithm>
#include <cctype>
#include <functional>
#include <fstream>
#include <iostream>
#include <limits>
#include <numeric>

#include "text.h"

#define FILE_TAG 81C76E7D

#ifndef ZXTUNE_VERSION
#define ZXTUNE_VERSION develop
#endif

#define STR(a) #a

#define VERSION_STRING(a) STR(a)

namespace
{
  const std::size_t INFORMATION_HEIGHT = 6;
  const std::size_t TRACKING_HEIGHT = 4;
  const std::size_t PLAYING_HEIGHT = 2;
  
  inline String GetDefaultConfigFile()
  {
#ifdef _WIN32
    const String configPath(TEXT_CONFIG_PATH_WIN);
    if (const char* homeDir = ::getenv(ToStdString(TEXT_ENV_HOMEDIR_WIN).c_str()))
#else
    const String configPath(TEXT_CONFIG_PATH_NIX);
    if (const char* homeDir = ::getenv(ToStdString(TEXT_ENV_HOMEDIR_NIX).c_str()))
#endif
    {
      return ZXTune::IO::AppendPath(FromStdString(homeDir), configPath);
    }
    return TEXT_CONFIG_FILENAME;
  }
  
  void ReadConfigFile(const String& filename, Parameters::Map& params)
  {
    const String configName(filename.empty() ? TEXT_CONFIG_FILENAME : filename);
    
    typedef std::basic_ifstream<Char> FileStream;
    std::auto_ptr<FileStream> configFile(new FileStream(configName.c_str()));
    if (!*configFile)
    {
      if (!filename.empty())
      {
        throw Error(THIS_LINE, CONFIG_FILE, TEXT_ERROR_CONFIG_FILE);
      }
      configFile.reset(new FileStream(GetDefaultConfigFile().c_str()));
    }
    if (!*configFile)
    {
      params.clear();
      return;
    }

    String lines;
    std::vector<Char> buffer(1024);
    for (;;)
    {
      configFile->getline(&buffer[0], buffer.size());
      if (const std::streamsize lineSize = configFile->gcount())
	    {
        std::vector<Char>::const_iterator endof(buffer.begin() + lineSize - 1);
        std::vector<Char>::const_iterator beginof(std::find_if<std::vector<Char>::const_iterator>(buffer.begin(), endof, 
          std::not1(std::ptr_fun<int, int>(&std::isspace))));
        if (beginof != endof && *beginof != Char('#'))
        {
          if (!lines.empty())
          {
            lines += Char(',');
          }
          lines += String(beginof, endof);
        }
      }
      else
      {
        break;
      }
    }
    if (lines.empty())
    {
      params.clear();
    }
    else
    {
      ThrowIfError(ParseParametersString(String(), lines, params));
    }
  }
  
  void ErrOuter(uint_t /*level*/, Error::LocationRef loc, Error::CodeType code, const String& text)
  {
    StdOut << Error::AttributesToString(loc, code, text);
  }

  void OutProp(const StringMap::value_type& prop)
  {
    StdOut << prop.first << '=' << prop.second << std::endl;
  }

  void ShowItemInfo(const ZXTune::Module::Information& info, uint_t frameDuration)
  {
    StringMap strProps;
    Parameters::ConvertMap(info.Properties, strProps);
#if 1
    const String& infoFmt(InstantiateTemplate(TEXT_ITEM_INFO, strProps, FILL_NONEXISTING));

    assert(INFORMATION_HEIGHT == std::count(infoFmt.begin(), infoFmt.end(), '\n'));
    StdOut << (Formatter(infoFmt)
      % UnparseFrameTime(info.Statistic.Frame, frameDuration) % info.PhysicalChannels).str();
#else
    std::for_each(strProps.begin(), strProps.end(), OutProp);
#endif
  }

  void ShowTrackingStatus(const ZXTune::Module::Tracking& track)
  {
    const String& dump = (Formatter(TEXT_TRACKING_STATUS)
      % track.Position % track.Pattern % track.Line % track.Frame % track.Tempo % track.Channels).str();
    assert(TRACKING_HEIGHT == std::count(dump.begin(), dump.end(), '\n'));
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
    String data((Formatter(TEXT_PLAYBACK_STATUS) % UnparseFrameTime(frame, frameDuration) % MARKER).str());
    const String::size_type totalSize = data.size() - 1 - PLAYING_HEIGHT;
    const String::size_type markerPos = data.find(MARKER);
    
    String prog(width - totalSize, '-');
    const uint_t pos = frame * (width - totalSize) / allframe;
    prog[pos] = StateSymbol(state);
    data.replace(markerPos, 1, prog);
    assert(PLAYING_HEIGHT == std::count(data.begin(), data.end(), '\n'));
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
      if (!Parameters::FindByName(paramsMap, CONVERSION_PARAM_MODE, mode))
      {
        throw Error(THIS_LINE, CONVERT_PARAMETERS, TEXT_CONVERT_ERROR_NO_MODE);
      }
      if (!Parameters::FindByName(paramsMap, CONVERSION_PARAM_FILENAME, NameTemplate))
      {
        throw Error(THIS_LINE, CONVERT_PARAMETERS, TEXT_CONVERT_ERROR_NO_FILENAME);
      }
      if (mode == CONVERSION_MODE_RAW)
      {
        ConversionParameter.reset(new ZXTune::Module::Conversion::RawConvertParam());
        CapabilityMask = ZXTune::CAP_CONV_RAW;
      }
      else if (mode == CONVERSION_MODE_PSG)
      {
        ConversionParameter.reset(new ZXTune::Module::Conversion::PSGConvertParam());
        CapabilityMask = ZXTune::CAP_CONV_PSG;
      }
      else
      {
        throw Error(THIS_LINE, CONVERT_PARAMETERS, TEXT_CONVERT_ERROR_INVALID_MODE);
      }
    }
    void ProcessItem(const ModuleItem& item) const
    {
      {
        ZXTune::PluginInformation info;
        item.Module->GetPluginInformation(info);
        if (!(info.Capabilities & CapabilityMask))
        {
          Message(TEXT_CONVERT_SKIPPED, item.Id, info.Id);
          return;
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
          TEXT_CONVERT_ERROR_WRITE_FILE, filename);
      }
      Message(TEXT_CONVERT_DONE, item.Id, filename);
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
      , Cached(false)
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
          if (Cached)
          {
            ModuleItemsArray playlist;
            Sourcer->ProcessItems(boost::bind(&ModuleItemsArray::push_back, boost::ref(playlist), _1));
            StdOut << "Detected " << playlist.size() << " items" << std::endl;
            std::for_each(playlist.begin(), playlist.end(), boost::bind(&CLIApplication::PlayItem, this, _1));
          }
          else
          {
            Sourcer->ProcessItems(boost::bind(&CLIApplication::PlayItem, this, _1));
          }
        }
      }
      catch (const Error& e)
      {
        if (!e.FindSuberror(CANCELED))
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
        options_description options((Formatter(TEXT_USAGE_SECTION) % *argv).str());
        options.add_options()
          (TEXT_HELP_KEY, TEXT_HELP_DESC)
          (TEXT_VERSION_KEY, TEXT_VERSION_DESC)
          (TEXT_CONFIG_KEY, boost::program_options::value<String>(&configFile), 
            (Formatter(TEXT_CONFIG_DESC) % GetDefaultConfigFile()).str().c_str())
          (TEXT_IO_PROVIDERS_OPTS_KEY, boost::program_options::value<String>(&providersOptions), TEXT_IO_PROVIDERS_OPTS_DESC)
          (TEXT_CORE_OPTS_KEY, boost::program_options::value<String>(&coreOptions), TEXT_CORE_OPTS_DESC)
          (TEXT_CONVERT_KEY, boost::program_options::value<String>(&ConvertParams), TEXT_CONVERT_DESC)
        ;
        
        options.add(Informer->GetOptionsDescription());
        options.add(Sourcer->GetOptionsDescription());
        options.add(Sounder->GetOptionsDescription());
        //add positional parameters for input
        positional_options_description inputPositional;
        inputPositional.add(TEXT_INPUT_FILE_KEY, -1);
        
        //cli options
        options_description cliOptions(TEXT_CLI_SECTION);
        cliOptions.add_options()
          (TEXT_SILENT_KEY, bool_switch(&Silent), TEXT_SILENT_DESC)
          (TEXT_QUIET_KEY, bool_switch(&Quiet), TEXT_QUIET_DESC)
          (TEXT_ANALYZER_KEY, bool_switch(&Analyzer), TEXT_ANALYZER_DESC)
          (TEXT_CACHE_KEY, bool_switch(&Cached), TEXT_CACHE_DESC)
          (TEXT_SEEKSTEP_KEY, value<uint_t>(&SeekStep), TEXT_SEEKSTEP_DESC)
          (TEXT_UPDATEFPS_KEY, value<uint_t>(&Updatefps), TEXT_UPDATEFPS_DESC)
        ;
        options.add(cliOptions);
        
        variables_map vars;
        store(command_line_parser(argc, argv).options(options).positional(inputPositional).run(), vars);
        notify(vars);
        if (vars.count(TEXT_HELP_KEY))
        {
          StdOut << options << std::endl;
          return true;
        }
        else if (vars.count(TEXT_VERSION_KEY))
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
          ReadConfigFile(configFile, configParams);
          Parameters::MergeMaps(GlobalParams, configParams, GlobalParams, false);
        }
        return false;
      }
      catch (const std::exception& e)
      {
        throw MakeFormattedError(THIS_LINE, UNKNOWN_ERROR, TEXT_COMMON_ERROR, e.what());
      }
    }
    
    void PlayItem(const ModuleItem& item)
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
      ZXTune::Module::Information info;
      item.Module->GetModuleInformation(info);

      //show startup info
      if (!Silent)
      {
        ShowItemInfo(info, static_cast<uint_t>(frameDuration));
      }
      const uint_t seekStepFrames(info.Statistic.Frame * SeekStep / 100);
      const uint_t waitPeriod(std::max<uint_t>(1, 1000 / std::max<uint_t>(Updatefps, 1)));
      ThrowIfError(backend.Play());
      std::vector<int_t> analyzer;
      Console::SizeType scrSize;
      ZXTune::Module::Player::ConstWeakPtr weakPlayer(backend.GetPlayer());
      ZXTune::Sound::Gain curVolume = ZXTune::Sound::Gain();
      ZXTune::Sound::MultiGain allVolume;
      const bool noVolume = backend.GetVolume(allVolume) != 0;
      if (!noVolume)
      {
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
          const int spectrumHeight = scrSize.second - INFORMATION_HEIGHT - TRACKING_HEIGHT - PLAYING_HEIGHT - 1;
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
            ShowPlaybackStatus(curFrame, info.Statistic.Frame, state, scrSize.first, frameDuration);
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
          case Console::KEY_CANCEL:
          case 'Q':
            throw Error(THIS_LINE, CANCELED);
            break;
          case Console::KEY_LEFT:
            ThrowIfError(backend.SetPosition(curFrame < seekStepFrames ? 0 : curFrame - seekStepFrames));
            break;
          case Console::KEY_RIGHT:
            ThrowIfError(backend.SetPosition(curFrame + seekStepFrames));
            break;
          case Console::KEY_DOWN:
            if (!noVolume)
            {
              curVolume = std::max(0.0, curVolume - 0.05);
              ZXTune::Sound::MultiGain allVol;
              allVol.assign(curVolume);
              ThrowIfError(backend.SetVolume(allVol));
            }
            break;
          case Console::KEY_UP:
            if (!noVolume)
            {
              curVolume = std::min(1.0, curVolume + 0.05);
              ZXTune::Sound::MultiGain allVol;
              allVol.assign(curVolume);
              ThrowIfError(backend.SetVolume(allVol));
            }
            break;
          case Console::KEY_ENTER:
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

        if (ZXTune::Sound::Backend::STOPPED == state ||
            ZXTune::Sound::Backend::STOP == backend.WaitForEvent(ZXTune::Sound::Backend::STOP, waitPeriod))
        {
          break;
        }
        if (!Silent && !Quiet)
        {
          Console::Self().MoveCursorUp(Analyzer ? scrSize.second - INFORMATION_HEIGHT - 1 : TRACKING_HEIGHT + PLAYING_HEIGHT);
        }
      }
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
    bool Cached;
    uint_t SeekStep;
    uint_t Updatefps;
  };
}

std::auto_ptr<Application> Application::Create()
{
  return std::auto_ptr<Application>(new CLIApplication());
}
