#include "app.h"
#include "console.h"
#include "error_codes.h"
#include "information.h"
#include "parsing.h"
#include "sound.h"
#include "source.h"

#include <error_tools.h>
#include <template.h>
#include <core/core_parameters.h>
#include <core/module_attrs.h>
#include <io/providers_parameters.h>

#include <boost/bind.hpp>
#include <boost/program_options.hpp>

#include <algorithm>
#include <functional>
#include <iostream>
#include <limits>

#include "cmdline.h"
#include "messages.h"

#define FILE_TAG 81C76E7D

namespace
{
  const int INFORMATION_HEIGHT = 5;
  const int TRACKING_HEIGHT = 4;
  const int PLAYING_HEIGHT = 2;
  
  void ErrOuter(unsigned /*level*/, Error::LocationRef loc, Error::CodeType code, const String& text)
  {
    std::cout << Error::AttributesToString(loc, code, text);
  }

  void ShowItemInfo(const String& id, const ZXTune::Module::Information& info)
  {
    StringMap strProps;
    Parameters::ConvertMap(info.Properties, strProps);
    const String& infoFmt(InstantiateTemplate(TEXT_ITEM_INFO, strProps, FILL_NONEXISTING));

    assert(INFORMATION_HEIGHT == std::count(infoFmt.begin(), infoFmt.end(), '\n'));
    std::cout << (Formatter(infoFmt)
      % id % UnparseFrameTime(info.Statistic.Frame, 20000) % info.PhysicalChannels).str();
  }
  
  void ShowTrackingStatus(const ZXTune::Module::Tracking& track)
  {
    const String& dump = (Formatter(TEXT_TRACKING_STATUS)
      % track.Position % track.Pattern % track.Line % track.Frame % track.Tempo % track.Channels).str();
    assert(TRACKING_HEIGHT == std::count(dump.begin(), dump.end(), '\n'));
    std::cout << dump;
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
  
  void ShowPlaybackStatus(unsigned frame, unsigned allframe, ZXTune::Sound::Backend::State state, unsigned width)
  {
    const Char MARKER = '\x1';
    String data((Formatter(TEXT_PLAYBACK_STATUS) % UnparseFrameTime(frame, 20000) % MARKER).str());
    const String::size_type totalSize = data.size() - 1 - PLAYING_HEIGHT;
    const String::size_type markerPos = data.find(MARKER);
    
    String prog(width - totalSize, '-');
    const unsigned pos = frame * (width - totalSize) / allframe;
    prog[pos] = StateSymbol(state);
    data.replace(markerPos, 1, prog);
    assert(PLAYING_HEIGHT == std::count(data.begin(), data.end(), '\n'));
    std::cout << data << std::flush;
  }
  
  void UpdateAnalyzer(const ZXTune::Module::Analyze::ChannelsState& inState,
    unsigned fallspeed, std::vector<int>& outState)
  {
    std::transform(outState.begin(), outState.end(), outState.begin(),
      std::bind2nd(std::minus<int>(), fallspeed));
    for (unsigned chan = 0, lim = static_cast<unsigned>(inState.size()); chan != lim; ++chan)
    {
      const ZXTune::Module::Analyze::Channel& state(inState[chan]);
      if (state.Enabled && state.Band < outState.size())
      {
        outState[state.Band] = state.Level;
      }
    }
    std::replace_if(outState.begin(), outState.end(), std::bind2nd(std::less<int>(), 0), 0);
  }
  
  inline char SymIfGreater(int val, int limit)
  {
    return val > limit ? '#' : ' ';
  }
  
  void ShowAnalyzer(const std::vector<int>& state, unsigned high)
  {
    const unsigned width = static_cast<unsigned>(state.size());
    std::string buffer(width, ' ');
    for (unsigned y = high; y; --y)
    {
      const int limit = (y - 1) * std::numeric_limits<ZXTune::Module::Analyze::LevelType>::max() / high;
      std::transform(state.begin(), state.end(), buffer.begin(), boost::bind(SymIfGreater, _1, limit));
      std::cout << buffer << '\n';
    }
    std::cout << std::flush;
  }
  
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
        
        if (Cached)
        {
          ModuleItemsArray playlist;
          Sourcer->ProcessItems(boost::bind(&ModuleItemsArray::push_back, boost::ref(playlist), _1));
          std::cout << "Detected " << playlist.size() << " items" << std::endl;
          std::for_each(playlist.begin(), playlist.end(), boost::bind(&CLIApplication::PlayItem, this, _1));
        }
        else
        {
          Sourcer->ProcessItems(boost::bind(&CLIApplication::PlayItem, this, _1));
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

        String providersOptions, coreOptions;
        options_description options((Formatter(TEXT_USAGE_SECTION) % *argv).str());
        options.add_options()
          (TEXT_HELP_KEY, TEXT_HELP_DESC)
          (TEXT_IO_PROVIDERS_OPTS_KEY, boost::program_options::value<String>(&providersOptions), TEXT_IO_PROVIDERS_OPTS_DESC)
          (TEXT_CORE_OPTS_KEY, boost::program_options::value<String>(&coreOptions), TEXT_CORE_OPTS_DESC)
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
          (TEXT_CACHE_KEY, bool_switch(&Cached), TEXT_CACHE_DESC);
        ;
        options.add(cliOptions);
        
        variables_map vars;
        store(command_line_parser(argc, argv).options(options).positional(inputPositional).run(), vars);
        notify(vars);
        if (vars.count(TEXT_HELP_KEY))
        {
          std::cout << options << std::endl;
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
        return false;
      }
      catch (const std::exception& e)
      {
        throw MakeFormattedError(THIS_LINE, UNKNOWN_ERROR, TEXT_COMMON_ERROR, e.what());
      }
    }
    
    void PlayItem(const ModuleItem& item)
    {
      ZXTune::Module::Information info;
      item.Module->GetModuleInformation(info);

      //show startup info
      if (!Silent)
      {
        ShowItemInfo(item.Id, info);
      }
      //calculate and apply parameters
      Parameters::Map perItemParams(GlobalParams);
      perItemParams.insert(item.Params.begin(), item.Params.end());
      ZXTune::Sound::Backend& backend(Sounder->GetBackend());
      ThrowIfError(backend.SetModule(item.Module));
      ThrowIfError(backend.SetParameters(perItemParams));
      ThrowIfError(backend.Play());

      const unsigned seekStep(info.Statistic.Frame / 20);

      std::vector<int> analyzer;
      std::pair<int, int> scrSize;
      ZXTune::Module::Player::ConstWeakPtr weakPlayer(backend.GetPlayer());
      for (;;)
      {
        if (!Silent && !Quiet)
        {
          GetConsoleSize(scrSize);
          if (scrSize.first <= 0 || scrSize.second <= 0)
          {
            Silent = true;
          }
        }
        ZXTune::Sound::Backend::State state;
        ThrowIfError(backend.GetCurrentState(state));

        unsigned curFrame = 0;
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
          else if (scrSize.second < TRACKING_HEIGHT + PLAYING_HEIGHT)
          {
            Quiet = true;
          }
          else
          {
            ShowTrackingStatus(curTracking);
            ShowPlaybackStatus(curFrame, info.Statistic.Frame, state, scrSize.first);
            if (Analyzer)
            {
              analyzer.resize(scrSize.first);
              UpdateAnalyzer(curAnalyze, 10, analyzer);
              ShowAnalyzer(analyzer, spectrumHeight);
            }
          }
        }
        if (const unsigned key = GetPressedKey())
        {
          switch (key)
          {
          case KEY_CANCEL:
          case 'Q':
            throw Error(THIS_LINE, CANCELED);
            break;
          case KEY_LEFT:
            ThrowIfError(backend.SetPosition(curFrame < seekStep ? 0 : curFrame - seekStep));
            break;
          case KEY_RIGHT:
            ThrowIfError(backend.SetPosition(curFrame + seekStep));
            break;
          case KEY_ENTER:
            if (ZXTune::Sound::Backend::STARTED == state)
            {
              ThrowIfError(backend.Pause());
              while (GetPressedKey()) {};
            }
            else
            {
              while (GetPressedKey()) {};
              ThrowIfError(backend.Play());
            }
            break;
          case ' ':
            ThrowIfError(backend.Stop());
            state = ZXTune::Sound::Backend::STOPPED;
            while (GetPressedKey()) {};
            break;
          }
        }

        if (ZXTune::Sound::Backend::STOPPED == state ||
            ZXTune::Sound::Backend::STOP == backend.WaitForEvent(ZXTune::Sound::Backend::STOP, 100))
        {
          break;
        }
        if (!Silent && !Quiet)
        {
          MoveCursorUp(Analyzer ? scrSize.second - INFORMATION_HEIGHT - 1 : TRACKING_HEIGHT + PLAYING_HEIGHT);
        }
      }
    }
  private:
    Parameters::Map GlobalParams;
    std::auto_ptr<InformationComponent> Informer;
    std::auto_ptr<SourceComponent> Sourcer;
    std::auto_ptr<SoundComponent> Sounder;
    bool Silent;
    bool Quiet;
    bool Analyzer;
    bool Cached;
  };
}

std::auto_ptr<Application> Application::Create()
{
  return std::auto_ptr<Application>(new CLIApplication());
}
