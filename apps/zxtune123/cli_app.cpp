#include "app.h"
#include "error_codes.h"
#include "information.h"
#include "parsing.h"
#include "sound.h"
#include "source.h"

#include <error_tools.h>
#include <template.h>
#include <core/module_attrs.h>

#include <boost/bind.hpp>
#include <boost/program_options.hpp>

#include <algorithm>
#include <functional>
#include <iostream>
#include <limits>

//platform-dependend
#ifdef __linux__
#include <errno.h>
#include <termio.h>
#endif

#include "messages.h"

#define FILE_TAG 81C76E7D

namespace
{
  const unsigned INFORMATION_HEIGHT = 5;
  const unsigned TRACKING_HEIGHT = 4;
  const unsigned PLAYING_HEIGHT = 2;
  
#ifdef __linux__
  void ThrowIfError(int res, Error::LocationRef loc)
  {
    if (res)
    {
      throw Error(loc, UNKNOWN_ERROR, ::strerror(errno));
    }
  }
  
  void GetConsoleSize(std::pair<int, int>& sizes)
  {
#if defined TIOCGSIZE
    struct ttysize ts;
    ThrowIfError(ioctl(STDOUT_FILENO, TIOCGSIZE, &ts), THIS_LINE);
    sizes = std::make_pair(ts.ts_cols, ts.rs_rows);
#elif defined TIOCGWINSZ
    struct winsize ws;
    ThrowIfError(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws), THIS_LINE);
    sizes = std::make_pair(ws.ws_col, ws.ws_row);
#else
#error Unknown console mode for linux
#endif
  }
  
  void MoveCursorUp(int lines)
  {
    std::cout << "\x1b[" << lines << 'A' << std::flush;
  }
#endif
  
  void ErrOuter(unsigned /*level*/, Error::LocationRef loc, Error::CodeType code, const String& text)
  {
    std::cout << Error::AttributesToString(loc, code, text);
  }

  void ShowItemInfo(const String& id, const ZXTune::Module::Information& info)
  {
    StringMap strProps;
    Parameters::ConvertMap(info.Properties, strProps);
    const String& infoFmt(InstantiateTemplate(
        "Playing: %1%\n"
        "Type:  [Type]\t"  "Container: [Container]\n"
        "Title: [Title]\t" "Program:   [Program]\n"
        "Time:  %2%\t"     "Channels:  %3%\n"
        "\n", strProps, FILL_NONEXISTING));

    assert(INFORMATION_HEIGHT == std::count(infoFmt.begin(), infoFmt.end(), '\n'));
    std::cout << (Formatter(infoFmt)
      % id % UnparseFrameTime(info.Statistic.Frame, 20000) % info.PhysicalChannels).str();
  }
  
  void ShowTrackingStatus(const ZXTune::Module::Tracking& track)
  {
    const String& dump = (Formatter(
      "Position: %1%  \t\tPattern:  %2%  \n"
      "Line:     %3%  \t\tFrame:    %4%  \n"
      "Tempo:    %5%  \t\tChannels: %6%  \n"
      "\n"
      )
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
    String data((Formatter(
      "[%1%] [%2%]\n\n") % UnparseFrameTime(frame, 20000) % MARKER).str());
    const String::size_type totalSize = data.size() - 1 - PLAYING_HEIGHT;
    const String::size_type markerPos = data.find(MARKER);
    
    String prog(width - totalSize, '-');
    const unsigned pos = frame * (width - totalSize - 1) / allframe;
    prog[pos + 1] = StateSymbol(state);
    data.replace(markerPos, 1, prog);
    assert(PLAYING_HEIGHT == std::count(data.begin(), data.end(), '\n'));
    std::cout << data << std::flush;
  }
  
  void UpdateAnalyzer(const ZXTune::Module::Analyze::ChannelsState& inState,
    unsigned fallspeed, std::vector<int>& outState)
  {
    std::transform(outState.begin(), outState.end(), outState.begin(),
      std::bind2nd(std::minus<int>(), fallspeed));
    for (unsigned chan = 0, lim = inState.size(); chan != lim; ++chan)
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
        
        ModuleItemsArray playlist;
        Sourcer->GetItems(playlist);
        std::for_each(playlist.begin(), playlist.end(), boost::bind(&CLIApplication::PlayItem, this, _1));
      }
      catch (const Error& e)
      {
        e.WalkSuberrors(ErrOuter);
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

        options_description options((Formatter(TEXT_USAGE_SECTION) % *argv).str());
        options.add_options()(TEXT_HELP_KEY, TEXT_HELP_DESC);
        options.add(Informer->GetOptionsDescription());
        options.add(Sourcer->GetOptionsDescription());
        options.add(Sounder->GetOptionsDescription());
        //add positional parameters for input
        positional_options_description inputPositional;
        inputPositional.add(TEXT_INPUT_FILE_KEY, -1);
        
        variables_map vars;
        store(command_line_parser(argc, argv).options(options).positional(inputPositional).run(), vars);
        notify(vars);
        if (vars.count(TEXT_HELP_KEY))
        {
          std::cout << options << std::endl;
          return true;
        }
        return false;
      }
      catch (const std::exception& e)
      {
        throw MakeFormattedError(THIS_LINE, UNKNOWN_ERROR, "Error: %1%", e.what());
      }
    }
    
    void PlayItem(const ModuleItem& item)
    {
      ZXTune::Module::Information info;
      item.Module->GetModuleInformation(info);

      //show startup info
      ShowItemInfo(item.Id, info);
      //calculate and apply parameters
      Parameters::Map perItemParams(GlobalParams);
      perItemParams.insert(item.Params.begin(), item.Params.end());
      ZXTune::Sound::Backend& backend(Sounder->GetBackend());
      ThrowIfError(backend.SetModule(item.Module));
      ThrowIfError(backend.SetParameters(perItemParams));
      
      ThrowIfError(backend.Play());
      
      std::vector<int> analyzer;
      std::pair<int, int> scrSize;
      while (ZXTune::Module::Player::ConstPtr player = backend.GetPlayer().lock()) 
      {
        GetConsoleSize(scrSize);
        analyzer.resize(scrSize.first);
        ZXTune::Sound::Backend::State state;
        ThrowIfError(backend.GetCurrentState(state));
        
        unsigned curFrame = 0;
        ZXTune::Module::Tracking curTracking;
        ZXTune::Module::Analyze::ChannelsState curAnalyze;
        ThrowIfError(player->GetPlaybackState(curFrame, curTracking, curAnalyze));
        
        ShowTrackingStatus(curTracking);
        ShowPlaybackStatus(curFrame, info.Statistic.Frame, state, scrSize.first);
        
        UpdateAnalyzer(curAnalyze, 10, analyzer);
        ShowAnalyzer(analyzer, scrSize.second - INFORMATION_HEIGHT - TRACKING_HEIGHT - PLAYING_HEIGHT - 1);
        
        if (ZXTune::Sound::Backend::STOP == backend.WaitForEvent(ZXTune::Sound::Backend::STOP, 100))
        {
          break;
        }
        MoveCursorUp(scrSize.second - INFORMATION_HEIGHT - 1);
      }
    }
  private:
    Parameters::Map GlobalParams;
    std::auto_ptr<InformationComponent> Informer;
    std::auto_ptr<SourceComponent> Sourcer;
    std::auto_ptr<SoundComponent> Sounder;
  };
}

std::auto_ptr<Application> Application::Create()
{
  return std::auto_ptr<Application>(new CLIApplication());
}
