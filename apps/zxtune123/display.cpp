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
#include <template_parameters.h>
//library includes
#include <strings/format.h>
#include <strings/template.h>
#include <time/duration.h>
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

  inline void ShowTrackingStatus(const ZXTune::Module::TrackState& state)
  {
    const String& dump = Strings::Format(Text::TRACKING_STATUS,
      state.Position(), state.Pattern(),
      state.Line(), state.Quirk(),
      state.Channels(), state.Tempo());
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
      , ShowAnalyze(false)
      , Updatefps(10)
      , InformationTemplate(Strings::Template::Create(Text::ITEM_INFO))
      , ScrSize(Console::Self().GetSize())
      , TotalFrames(0)
      , FrameDuration()
    {
      using namespace boost::program_options;
      Options.add_options()
        (Text::SILENT_KEY, bool_switch(&Silent), Text::SILENT_DESC)
        (Text::QUIET_KEY, bool_switch(&Quiet), Text::QUIET_DESC)
        (Text::ANALYZER_KEY, bool_switch(&ShowAnalyze), Text::ANALYZER_DESC)
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

    virtual void SetModule(ZXTune::Sound::Backend::Ptr player, uint_t frameDuration)
    {
      const ZXTune::Module::Information::Ptr info = player->GetModuleInformation();
      const Parameters::Accessor::Ptr props = player->GetModuleProperties();
      TotalFrames = info->FramesCount();
      FrameDuration = Time::Microseconds(frameDuration);
      TrackState = player->GetTrackState();
      if (!Silent && ShowAnalyze)
      {
        Analyzer = player->GetAnalyzer();
      }
      else
      {
        Analyzer.reset();
      }

      if (Silent)
      {
        return;
      }
      StdOut
        << std::endl
        << InformationTemplate->Instantiate(Parameters::FieldsSourceAdapter<Strings::FillFieldsSource>(*props))
        << Strings::Format(Text::ITEM_INFO_ADDON, Time::MicrosecondsDuration(info->FramesCount(), FrameDuration).ToString(),
          info->LogicalChannels(), info->PhysicalChannels());
    }

    virtual uint_t BeginFrame(ZXTune::Sound::Backend::State state)
    {
      const uint_t curFrame = TrackState->Frame();
      if (Silent || Quiet)
      {
        return curFrame;
      }
      ScrSize = Console::Self().GetSize();
      if (ScrSize.first <= 0 || ScrSize.second <= 0)
      {
        Silent = true;
        return curFrame;
      }
      const int_t spectrumHeight = ScrSize.second - INFORMATION_HEIGHT - TRACKING_HEIGHT - PLAYING_HEIGHT - 1;
      if (spectrumHeight < 4)//minimal spectrum height
      {
        Analyzer.reset();
      }
      else if (ScrSize.second < int_t(TRACKING_HEIGHT + PLAYING_HEIGHT))
      {
        Quiet = true;
      }
      else
      {
        ShowTrackingStatus(*TrackState);
        ShowPlaybackStatus(curFrame, state);
        if (Analyzer)
        {
          std::vector<ZXTune::Module::Analyzer::BandAndLevel> curAnalyze;
          Analyzer->BandLevels(curAnalyze);

          AnalyzerData.resize(ScrSize.first);
          UpdateAnalyzer(curAnalyze, 10);
          ShowAnalyzer(spectrumHeight);
        }
      }
      return curFrame;
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
      String data = Strings::Format(Text::PLAYBACK_STATUS, Time::MicrosecondsDuration(frame, FrameDuration).ToString(), MARKER);
      const String::size_type totalSize = data.size() - 1 - PLAYING_HEIGHT;
      const String::size_type markerPos = data.find(MARKER);

      String prog(ScrSize.first - totalSize, '-');
      const std::size_t pos = frame * (ScrSize.first - totalSize) / TotalFrames;
      prog[pos] = StateSymbol(state);
      data.replace(markerPos, 1, prog);
      assert(PLAYING_HEIGHT == static_cast<std::size_t>(std::count(data.begin(), data.end(), '\n')));
      StdOut << data << std::flush;
    }

    void ShowAnalyzer(uint_t high)
    {
      const std::size_t width = AnalyzerData.size();
      std::string buffer(width, ' ');
      for (int_t y = high; y; --y)
      {
        const int_t limit = (y - 1) * 100 / high;
        std::transform(AnalyzerData.begin(), AnalyzerData.end(), buffer.begin(), boost::bind(&SymIfGreater, _1, limit));
        StdOut << buffer << '\n';
      }
      StdOut << std::flush;
    }

    void UpdateAnalyzer(const std::vector<ZXTune::Module::Analyzer::BandAndLevel>& inState, int_t fallspeed)
    {
      std::transform(AnalyzerData.begin(), AnalyzerData.end(), AnalyzerData.begin(),
        std::bind2nd(std::minus<int_t>(), fallspeed));
      for (std::vector<ZXTune::Module::Analyzer::BandAndLevel>::const_iterator it = inState.begin(), lim = inState.end(); it != lim; ++it)
      {
        if (it->first < AnalyzerData.size())
        {
          AnalyzerData[it->first] = it->second;
        }
      }
      std::replace_if(AnalyzerData.begin(), AnalyzerData.end(), std::bind2nd(std::less<int_t>(), 0), 0);
    }

  private:
    boost::program_options::options_description Options;
    bool Silent;
    bool Quiet;
    bool ShowAnalyze;
    uint_t Updatefps;
    const Strings::Template::Ptr InformationTemplate;
    //context
    Console::SizeType ScrSize;
    uint_t TotalFrames;
    Time::Microseconds FrameDuration;
    ZXTune::Module::TrackState::Ptr TrackState;
    ZXTune::Module::Analyzer::Ptr Analyzer;
    std::vector<int_t> AnalyzerData;
  };
}

DisplayComponent::Ptr DisplayComponent::Create()
{
  return DisplayComponent::Ptr(new DisplayComponentImpl);
}
