/**
 *
 * @file
 *
 * @brief Display component implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune123/display.h"

#include "apps/zxtune123/console.h"

#include "module/information.h"
#include "module/track_state.h"
#include "parameters/template.h"
#include "platform/application.h"
#include "sound/analyzer.h"
#include "strings/fields.h"
#include "strings/format.h"
#include "strings/template.h"
#include "time/duration.h"
#include "time/serialize.h"

#include "string_view.h"
#include "types.h"

#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>

#include <algorithm>
#include <chrono>
#include <ostream>
#include <thread>
#include <vector>

namespace
{
  template<class T>
  constexpr std::size_t CountLines(const T& str)
  {
    // TODO: use std::count at C++20
    std::size_t result = 0;
    for (auto c : str)
    {
      result += c == '\n';
    }
    return result;
  }

  // clang-format off
  constexpr auto ITEM_INFO =
      "Playing: [Fullpath]\n"
      "Type:    [Type]\tContainer: [Container]\tProgram: [Program]\n"
      "Title:   [Title]\n"
      "Author:  [Author]\n"sv;
  // Exact type for Strings::Format
  constexpr std::string_view ITEM_INFO_ADDON = "\nTime:    {0}\tLoop duration:  {1}\n\n";
  constexpr std::string_view TRACKING_FORMAT =
        "Position: {0:<6}Line:     {2:<6}Channels: {4:<6}\n"
        "Pattern:  {1:<6}Frame:    {3:<6}Tempo:    {5:<6}\n"
        "\n";
  constexpr std::string_view PLAYBACK_STATUS = "[{0}] [{1}]\n";
  // clang-format on

  // layout constants
  constexpr auto INFORMATION_HEIGHT = CountLines(ITEM_INFO) + CountLines(ITEM_INFO_ADDON);
  constexpr auto TRACKING_HEIGHT = CountLines(TRACKING_FORMAT);
  constexpr auto PLAYING_HEIGHT = CountLines(PLAYBACK_STATUS);

  class DisplayComponentImpl : public DisplayComponent
  {
  public:
    DisplayComponentImpl()
      : Options("Display-related options")
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

    void Message(StringView msg) override
    {
      if (!Silent)
      {
        Console::Self().Write(msg);
        StdOut << std::endl;
      }
    }

    void SetModule(Module::Holder::Ptr module, Sound::Backend::Ptr player) override
    {
      const auto info = module->GetModuleInformation();
      const auto props = module->GetModuleProperties();
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

      if (!Silent)
      {
        // May require encoding, so write via console
        Console::Self().Write(
            InformationTemplate->Instantiate(Parameters::FieldsSourceAdapter<Strings::FillFieldsSource>(*props)));
        StdOut << Strings::Format(ITEM_INFO_ADDON, Time::ToString(TotalDuration), Time::ToString(info->LoopDuration()));
      }
      DynamicLines = 0;
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
          UpdateAnalyzer(spectrum);
          ShowAnalyzer(spectrumHeight);
        }
        StdOut << std::flush;
      }
      return curPos;
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
      if (DynamicLines)
      {
        Console::Self().MoveCursorUp(DynamicLines);
      }
      DynamicLines = 0;
    }

    void ShowTrackingStatus(const Module::TrackState& state)
    {
      StdOut << Strings::Format(TRACKING_FORMAT, state.Position(), state.Pattern(), state.Line(), state.Quirk(),
                                state.Channels(), state.Tempo());
      DynamicLines += TRACKING_HEIGHT;
    }

    void ShowPlaybackStatus(Time::Milliseconds played, Sound::PlaybackControl::State state)
    {
      const auto MARKER = '\x1';
      String data = Strings::Format(PLAYBACK_STATUS, Time::ToString(played), MARKER);
      const String::size_type totalSize = data.size() - 1 - PLAYING_HEIGHT;
      const String::size_type markerPos = data.find(MARKER);

      String prog(ScrSize.first - totalSize, '-');
      const auto pos = (played * (ScrSize.first - totalSize)).Divide<uint_t>(TotalDuration);
      prog[pos] = StateSymbol(state);
      data.replace(markerPos, 1, prog);
      StdOut << data;
      DynamicLines += PLAYING_HEIGHT;
    }

    static char StateSymbol(Sound::PlaybackControl::State state)
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

    void ShowAnalyzer(uint_t high)
    {
      const std::size_t width = AnalyzerData.size();
      std::string buffer(width, ' ');
      for (int_t y = high; y; --y)
      {
        const int_t limit = (y - 1) * Sound::Analyzer::LevelType::PRECISION / high;
        std::transform(AnalyzerData.begin(), AnalyzerData.end(), buffer.begin(),
                       [limit](const int_t val) { return val > limit ? '#' : ' '; });
        StdOut << buffer << '\n';
      }
      DynamicLines += high;
    }

    void UpdateAnalyzer(const Sound::Analyzer::LevelType* inState)
    {
      const auto falling = Sound::Analyzer::LevelType::PRECISION / Updatefps;
      for (uint_t band = 0, lim = AnalyzerData.size(); band < lim; ++band)
      {
        AnalyzerData[band] = std::max<int>(AnalyzerData[band] - falling, int_t(inState[band].Raw()));
      }
    }

  private:
    boost::program_options::options_description Options;
    bool Silent = false;
    bool Quiet = false;
    bool ShowAnalyze = false;
    uint_t Updatefps = 10;
    const Strings::Template::Ptr InformationTemplate;
    // context
    std::chrono::time_point<std::chrono::steady_clock> NextFrameStart;
    std::size_t DynamicLines = 0;
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
