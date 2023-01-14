/**
 *
 * @file
 *
 * @brief Display component implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "display.h"
#include "console.h"
// common includes
#include <error.h>
// library includes
#include <module/track_state.h>
#include <parameters/template.h>
#include <platform/application.h>
#include <strings/format.h>
#include <strings/template.h>
#include <time/duration.h>
#include <time/serialize.h>
// std includes
#include <thread>
// boost includes
#include <boost/program_options.hpp>

namespace
{
  // layout constants
  // TODO: make dynamic calculation
  const std::size_t INFORMATION_HEIGHT = 5;
  const std::size_t TRACKING_HEIGHT = 3;
  const std::size_t PLAYING_HEIGHT = 2;

  inline void ShowTrackingStatus(const Module::TrackState& state)
  {
    static const Char FORMAT[] =
        "Position: {0:<6}Line:     {2:<6}Channels: {4:<6}\n"
        "Pattern:  {1:<6}Frame:    {3:<6}Tempo:    {5:<6}\n"
        "\n";

    const String& dump = Strings::Format(FORMAT, state.Position(), state.Pattern(), state.Line(), state.Quirk(),
                                         state.Channels(), state.Tempo());
    assert(TRACKING_HEIGHT == static_cast<std::size_t>(std::count(dump.begin(), dump.end(), '\n')));
    StdOut << dump;
  }

  inline Char StateSymbol(Sound::PlaybackControl::State state)
  {
    switch (state)
    {
    case Sound::PlaybackControl::STARTED:
      return '>';
    case Sound::PlaybackControl::PAUSED:
      return '#';
    default:
      return '\?';
    }
  }

  // clang-format off
  const Char ITEM_INFO[] =
      "Playing: [Fullpath]\n"
      "Type:    [Type]\tContainer: [Container]\tProgram: [Program]\n"
      "Title:   [Title]\n"
      "Author:  [Author]";

  const Char ITEM_INFO_ADDON[] = "\nTime:    {0}\tLoop duration:  {1}\n";

  const Char PLAYBACK_STATUS[] = "[{0}] [{1}]\n\n";
  // clang-format on

  class DisplayComponentImpl : public DisplayComponent
  {
  public:
    DisplayComponentImpl()
      : Options("Display-related options")
      , Silent(false)
      , Quiet(false)
      , ShowAnalyze(false)
      , Updatefps(10)
      , InformationTemplate(Strings::Template::Create(ITEM_INFO))
      , ScrSize(Console::Self().GetSize())
    {
      using namespace boost::program_options;
      auto opt = Options.add_options();
      opt("silent", bool_switch(&Silent), "disable all output");
      opt("quiet", bool_switch(&Quiet), "disable dynamic output");
      opt("analyzer", bool_switch(&ShowAnalyze), "enable spectrum analyzer");
      opt("updatefps", value<uint_t>(&Updatefps), "update rate");
    }

    const boost::program_options::options_description& GetOptionsDescription() const override
    {
      return Options;
    }

    void Message(const String& msg) override
    {
      if (!Silent)
      {
        Console::Self().Write(msg);
        StdOut << std::endl;
      }
    }

    void SetModule(Module::Holder::Ptr module, Sound::Backend::Ptr player) override
    {
      const Module::Information::Ptr info = module->GetModuleInformation();
      const Parameters::Accessor::Ptr props = module->GetModuleProperties();
      TotalDuration = info->Duration();
      State = player->GetState();
      TrackState = dynamic_cast<const Module::TrackState*>(State.get());
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
      Message(InformationTemplate->Instantiate(Parameters::FieldsSourceAdapter<Strings::FillFieldsSource>(*props)));
      Message(Strings::Format(ITEM_INFO_ADDON, Time::ToString(TotalDuration), Time::ToString(info->LoopDuration())));
      // TODO: also dump track information
    }

    Time::AtMillisecond BeginFrame(Sound::PlaybackControl::State state) override
    {
      const auto curPos = State->At();
      if (Silent || Quiet)
      {
        return curPos;
      }
      ScrSize = Console::Self().GetSize();
      if (ScrSize.first <= 0 || ScrSize.second <= 0)
      {
        Silent = true;
        return curPos;
      }
      const int_t trackingHeight = TrackState ? TRACKING_HEIGHT : 0;
      const int_t spectrumHeight = ScrSize.second - INFORMATION_HEIGHT - trackingHeight - PLAYING_HEIGHT - 1;
      if (spectrumHeight < 4)  // minimal spectrum height
      {
        Analyzer.reset();
      }
      else if (ScrSize.second < int_t(trackingHeight + PLAYING_HEIGHT))
      {
        Quiet = true;
      }
      else
      {
        Vsync();
        if (TrackState)
        {
          ShowTrackingStatus(*TrackState);
        }
        ShowPlaybackStatus(Time::Milliseconds(curPos.CastTo<Time::Millisecond>().Get()), state);
        if (Analyzer)
        {
          Sound::Analyzer::LevelType spectrum[ScrSize.first];
          Analyzer->GetSpectrum(spectrum, ScrSize.first);
          AnalyzerData.resize(ScrSize.first);
          UpdateAnalyzer(spectrum, 10);
          ShowAnalyzer(spectrumHeight);
        }
      }
      return curPos;
    }

    void EndFrame() override
    {
      if (!Silent && !Quiet)
      {
        const int_t trackingHeight = TrackState ? TRACKING_HEIGHT : 0;
        Console::Self().MoveCursorUp(Analyzer ? ScrSize.second - INFORMATION_HEIGHT - 1
                                              : trackingHeight + PLAYING_HEIGHT);
      }
    }

  private:
    void Vsync()
    {
      if (NextFrameStart != decltype(NextFrameStart){})
      {
        std::this_thread::sleep_until(NextFrameStart);
      }
      else
      {
        NextFrameStart = std::chrono::steady_clock::now();
      }
      const uint_t waitPeriod(std::max<uint_t>(1, 1000 / std::max<uint_t>(Updatefps, 1)));
      NextFrameStart += std::chrono::milliseconds(waitPeriod);
    }

    void ShowPlaybackStatus(Time::Milliseconds played, Sound::PlaybackControl::State state) const
    {
      const Char MARKER = '\x1';
      String data = Strings::Format(PLAYBACK_STATUS, Time::ToString(played), MARKER);
      const String::size_type totalSize = data.size() - 1 - PLAYING_HEIGHT;
      const String::size_type markerPos = data.find(MARKER);

      String prog(ScrSize.first - totalSize, '-');
      const auto pos = (played * (ScrSize.first - totalSize)).Divide<uint_t>(TotalDuration);
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
        std::transform(AnalyzerData.begin(), AnalyzerData.end(), buffer.begin(),
                       [limit](const int_t val) { return val > limit ? '#' : ' '; });
        StdOut << buffer << '\n';
      }
      StdOut << std::flush;
    }

    void UpdateAnalyzer(const Sound::Analyzer::LevelType* inState, int_t fallspeed)
    {
      for (uint_t band = 0, lim = AnalyzerData.size(); band < lim; ++band)
      {
        AnalyzerData[band] = std::max(AnalyzerData[band] - fallspeed, int_t(inState[band].Raw()));
      }
    }

  private:
    boost::program_options::options_description Options;
    bool Silent;
    bool Quiet;
    bool ShowAnalyze;
    uint_t Updatefps;
    const Strings::Template::Ptr InformationTemplate;
    // context
    std::chrono::time_point<std::chrono::steady_clock> NextFrameStart;
    Console::SizeType ScrSize;
    Time::Milliseconds TotalDuration;
    Module::State::Ptr State;
    const Module::TrackState* TrackState;
    Sound::Analyzer::Ptr Analyzer;
    std::vector<int_t> AnalyzerData;
  };
}  // namespace

DisplayComponent::Ptr DisplayComponent::Create()
{
  return DisplayComponent::Ptr(new DisplayComponentImpl);
}
