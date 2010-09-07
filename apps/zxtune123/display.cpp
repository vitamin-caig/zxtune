/*
Abstract:
  Display component implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune123 application based on zxtune library
*/

//local includes
#include "console.h"
#include "display.h"
#include <apps/base/app.h>
//common includes
#include <error.h>
#include <formatter.h>
#include <template.h>
//boost includes
#include <boost/bind.hpp>
#include <boost/program_options.hpp>
#include <boost/thread/thread.hpp>
//text includes
#include "text/text.h"

namespace
{
  //layout constants
  //TODO: make dynamic calculation
  const std::size_t INFORMATION_HEIGHT = 5;
  const std::size_t TRACKING_HEIGHT = 3;
  const std::size_t PLAYING_HEIGHT = 2;

  inline void OutProp(const StringMap::value_type& prop)
  {
    StdOut << prop.first << '=' << prop.second << std::endl;
  }

  inline void ShowTrackingStatus(const ZXTune::Module::State& state)
  {
    const String& dump = (Formatter(Text::TRACKING_STATUS)
      % state.Track.Position % state.Track.Pattern
      % state.Track.Line % state.Track.Quirk
      % state.Track.Channels % state.Reference.Quirk
    ).str();
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

  inline char SymIfGreater(int_t val, int_t limit)
  {
    return val > limit ? '#' : ' ';
  }

  class DisplayComponentImpl : public DisplayComponent
  {
  public:
    DisplayComponentImpl()
      : Options(Text::DISPLAY_SECTION)
      , Silent(false)
      , Quiet(false)
      , Analyzer(false)
      , Updatefps(10)
      , ScrSize(Console::Self().GetSize())
      , TotalFrames(0)
      , FrameDuration(0)
    {
      using namespace boost::program_options;
      Options.add_options()
        (Text::SILENT_KEY, bool_switch(&Silent), Text::SILENT_DESC)
        (Text::QUIET_KEY, bool_switch(&Quiet), Text::QUIET_DESC)
        (Text::ANALYZER_KEY, bool_switch(&Analyzer), Text::ANALYZER_DESC)
        (Text::UPDATEFPS_KEY, value<uint_t>(&Updatefps), Text::UPDATEFPS_DESC)
      ;
    }

    virtual const boost::program_options::options_description& GetOptionsDescription() const
    {
      return Options;
    }

    virtual void Message(const String& msg)
    {
      if (!Silent)
      {
        StdOut << msg << std::endl;
      }
    }

    virtual void SetModule(const ZXTune::Module::Information& info, ZXTune::Module::Player::ConstWeakPtr player, uint_t frameDuration)
    {
      if (Silent)
      {
        return;
      }
      Player = player;
      TotalFrames = info.FramesCount();
      FrameDuration = frameDuration;

      StringMap strProps;
      info.Properties()->Convert(strProps);
#if 1
      StdOut
        << std::endl
        << InstantiateTemplate(Text::ITEM_INFO, strProps, FILL_NONEXISTING)
        << (Formatter(Text::ITEM_INFO_ADDON) % FormatTime(info.FramesCount(), frameDuration) %
          info.LogicalChannels() % info.PhysicalChannels()).str();
#else
      std::for_each(strProps.begin(), strProps.end(), &OutProp);
#endif
    }

    virtual uint_t BeginFrame(ZXTune::Sound::Backend::State state)
    {
      ZXTune::Module::State curState;
      ZXTune::Module::Analyze::ChannelsState curAnalyze;
      ThrowIfError(Player.lock()->GetPlaybackState(curState, curAnalyze));
      if (Silent || Quiet)
      {
        return curState.Frame;
      }
      ScrSize = Console::Self().GetSize();
      if (ScrSize.first <= 0 || ScrSize.second <= 0)
      {
        Silent = true;
        return curState.Frame;
      }
      const int_t spectrumHeight = ScrSize.second - INFORMATION_HEIGHT - TRACKING_HEIGHT - PLAYING_HEIGHT - 1;
      if (spectrumHeight < 4)//minimal spectrum height
      {
        Analyzer = false;
      }
      else if (ScrSize.second < int_t(TRACKING_HEIGHT + PLAYING_HEIGHT))
      {
        Quiet = true;
      }
      else
      {
        ShowTrackingStatus(curState);
        ShowPlaybackStatus(curState.Frame, state);
        if (Analyzer)
        {
          AnalyzerData.resize(ScrSize.first);
          UpdateAnalyzer(curAnalyze, 10);
          ShowAnalyzer(spectrumHeight);
        }
      }
      return curState.Frame;
    }

    virtual void EndFrame()
    {
      const uint_t waitPeriod(std::max<uint_t>(1, 1000 / std::max<uint_t>(Updatefps, 1)));
      boost::this_thread::sleep(boost::posix_time::milliseconds(waitPeriod));
      if (!Silent && !Quiet)
      {
        Console::Self().MoveCursorUp(Analyzer ? ScrSize.second - INFORMATION_HEIGHT - 1 : TRACKING_HEIGHT + PLAYING_HEIGHT);
      }
    }
  private:
    void ShowPlaybackStatus(uint_t frame, ZXTune::Sound::Backend::State state) const
    {
      const Char MARKER = '\x1';
      String data((Formatter(Text::PLAYBACK_STATUS) % FormatTime(frame, FrameDuration) % MARKER).str());
      const String::size_type totalSize = data.size() - 1 - PLAYING_HEIGHT;
      const String::size_type markerPos = data.find(MARKER);

      String prog(ScrSize.first - totalSize, '-');
      const uint_t pos = frame * (ScrSize.first - totalSize) / TotalFrames;
      prog[pos] = StateSymbol(state);
      data.replace(markerPos, 1, prog);
      assert(PLAYING_HEIGHT == static_cast<std::size_t>(std::count(data.begin(), data.end(), '\n')));
      StdOut << data << std::flush;
    }

    void ShowAnalyzer(uint_t high)
    {
      const uint_t width = AnalyzerData.size();
      std::string buffer(width, ' ');
      for (int_t y = high; y; --y)
      {
        const int_t limit = (y - 1) * std::numeric_limits<ZXTune::Module::Analyze::LevelType>::max() / high;
        std::transform(AnalyzerData.begin(), AnalyzerData.end(), buffer.begin(), boost::bind(&SymIfGreater, _1, limit));
        StdOut << buffer << '\n';
      }
      StdOut << std::flush;
    }

    void UpdateAnalyzer(const ZXTune::Module::Analyze::ChannelsState& inState, int_t fallspeed)
    {
      std::transform(AnalyzerData.begin(), AnalyzerData.end(), AnalyzerData.begin(),
        std::bind2nd(std::minus<int_t>(), fallspeed));
      for (uint_t chan = 0, lim = inState.size(); chan != lim; ++chan)
      {
        const ZXTune::Module::Analyze::Channel& state(inState[chan]);
        if (state.Enabled && state.Band < AnalyzerData.size())
        {
          AnalyzerData[state.Band] = state.Level;
        }
      }
      std::replace_if(AnalyzerData.begin(), AnalyzerData.end(), std::bind2nd(std::less<int_t>(), 0), 0);
    }

  private:
    boost::program_options::options_description Options;
    bool Silent;
    bool Quiet;
    bool Analyzer;
    uint_t Updatefps;
    //context
    Console::SizeType ScrSize;
    ZXTune::Module::Player::ConstWeakPtr Player;
    uint_t TotalFrames;
    uint_t FrameDuration;
    std::vector<int_t> AnalyzerData;
  };
}

DisplayComponent::Ptr DisplayComponent::Create()
{
  return DisplayComponent::Ptr(new DisplayComponentImpl);
}
