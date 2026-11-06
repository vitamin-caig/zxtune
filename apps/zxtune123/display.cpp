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

#include "module/track_presentation.h"
#include "parameters/template.h"
#include "platform/application.h"
#include "strings/format.h"
#include "strings/split.h"
#include "strings/template.h"
#include "time/duration.h"
#include "time/serialize.h"

#include "contract.h"
#include "error.h"
#include "string_view.h"

#include <boost/program_options.hpp>

#include <algorithm>
#include <optional>
#include <thread>
#include <vector>

namespace
{
  enum class AnalyzerMode
  {
    NONE,
    SPECTRUM,
    TRACKING
  };

  AnalyzerMode ParseAnalyzerMode(StringView mode)
  {
    if (mode.empty())
    {
      return AnalyzerMode::NONE;
    }
    if (mode == "spectrum"sv || mode == "1"sv || mode == "true"sv)
    {
      return AnalyzerMode::SPECTRUM;
    }
    if (mode == "track"sv || mode == "tracking"sv)
    {
      return AnalyzerMode::TRACKING;
    }
    throw Error(THIS_LINE, "Unknown analyzer mode.");
  }

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
  constexpr auto LINE_POINTER = ">"sv;

  class PatternView
  {
  public:
    explicit PatternView(const Module::TrackPresentation& presentation)
      : Presentation(presentation)
    {}

    bool Update(const Module::TrackState& state, Console::SizeType size)
    {
      const auto width = size.first;
      const auto height = size.second;
      if (width <= 0 || height <= 0)
      {
        View.clear();
        ViewPosition.reset();
        return true;
      }
      const bool patternChanged = !ViewPosition || ViewPosition->Pattern != state.Pattern;
      if (patternChanged)
      {
        PatternText = Presentation.GetPattern(state.Pattern);
        if (PatternText.empty())
        {
          Lines.clear();
          View.clear();
          ViewPosition.reset();
          return false;
        }
        Lines = Strings::Split(PatternText, '\n');
        Require(Lines.size() == 1 + static_cast<std::size_t>(std::count(PatternText.begin(), PatternText.end(), '\n')));
      }
      if (ViewPosition && ViewPosition->Pattern == state.Pattern && ViewPosition->Line == state.Line
          && ViewSize == size)
      {
        return true;
      }
      const bool fullRedraw = patternChanged || ViewSize != size;
      View.clear();
      if (fullRedraw)
      {
        // Reused by partial redraws until the viewport size changes
        BlankLine.assign(width, ' ');
      }
      const auto total = static_cast<int_t>(Lines.size());
      const auto current = static_cast<int_t>(state.Line);
      const auto center = height / 2;
      const auto firstRow = current - center;
      const auto maxWidth =
          static_cast<std::size_t>(std::max<int_t>(0, width - static_cast<int_t>(LINE_POINTER.size())));
      const auto previousLine = ViewPosition ? static_cast<int_t>(ViewPosition->Line) : current;
      const auto lineShift = current - previousLine;
      const auto leading = std::min<int_t>(std::max<int_t>(0, -firstRow), height);
      View.resize(static_cast<std::size_t>(leading));
      std::fill(View.begin(), View.end(), fullRedraw ? StringView(BlankLine) : StringView{});
      if (!fullRedraw && lineShift < 0)
      {
        const auto clearBefore = static_cast<std::size_t>(std::min<int_t>(-lineShift, leading));
        std::fill(View.end() - clearBefore, View.end(), StringView(BlankLine));
      }
      const auto firstPatternRow = std::max<int_t>(firstRow, 0);
      const auto lastPatternRow = std::min<int_t>(firstRow + height, total);
      for (int_t row = firstPatternRow; row < lastPatternRow; ++row)
      {
        View.push_back(Lines[row].substr(0, maxWidth));
      }
      if (fullRedraw)
      {
        View.resize(static_cast<std::size_t>(height), BlankLine);
      }
      else if (lineShift > 0)
      {
        const auto tail = std::min<int_t>(lineShift, std::max<int_t>(0, height - static_cast<int_t>(View.size())));
        View.insert(View.end(), static_cast<std::size_t>(tail), BlankLine);
      }
      ViewPosition = state;
      ViewSize = size;
      return true;
    }

    int_t Draw()
    {
      if (View.empty())
      {
        return 0;
      }
      const auto total = static_cast<int_t>(Lines.size());
      const auto current = static_cast<int_t>(ViewPosition->Line);
      const auto center = ViewSize.second / 2;
      const auto count = static_cast<int_t>(View.size());
      const auto blankPointer = StringView{BlankLine}.substr(0, LINE_POINTER.size());
      for (int_t offset = 0; offset != count; ++offset)
      {
        const auto patternRow = current + offset - center;
        // The current-line marker is overlaid on the final output instead of being part of the view
        if (patternRow >= 0 && patternRow < total)
        {
          StdOut << (patternRow == current ? LINE_POINTER : blankPointer);
        }
        StdOut << View[offset] << '\n';
      }
      View.clear();
      return count;
    }

  private:
    const Module::TrackPresentation& Presentation;
    String PatternText;
    std::vector<StringView> Lines;
    std::vector<StringView> View;
    String BlankLine;
    std::optional<Module::TrackState> ViewPosition;
    Console::SizeType ViewSize{0, 0};
  };

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
      opt("analyzer", value<String>(&AnalyzerMode)->implicit_value("spectrum"),
          "enable analyzer: spectrum (default) or track");
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

    void SetModule(const Module::Holder& module, const Sound::Backend& player) override
    {
      const auto info = module.GetModuleInformation();
      const auto props = module.GetModuleProperties();
      TotalDuration = info.Duration;
      TrackingView.reset();
      Analyzer.reset();
      if (!Silent)
      {
        const auto analyzerMode = ParseAnalyzerMode(AnalyzerMode);
        if (analyzerMode != AnalyzerMode::NONE)
        {
          // Keep the spectrum available as a fallback for formats without track presentation
          Analyzer = player.GetAnalyzer();
        }
        if (analyzerMode == AnalyzerMode::TRACKING)
        {
          if (const auto* presentation = dynamic_cast<const Module::TrackPresentation*>(&module))
          {
            TrackingView.emplace(*presentation);
          }
        }
      }

      if (!Silent)
      {
        // May require encoding, so write via console
        Console::Self().Write(
            InformationTemplate->Instantiate(Parameters::FieldsSourceAdapter<Strings::FillFieldsSource>(*props)));
        StdOut << Strings::Format(ITEM_INFO_ADDON, Time::ToString(TotalDuration), Time::ToString(info.LoopDuration));
      }
      DynamicLines = 0;
    }

    void BeginFrame(Sound::PlaybackControl::PlaybackState playbackState, const Module::State& moduleState) override
    {
      if (Silent || Quiet)
      {
        return;
      }
      ScrSize = Console::Self().GetSize();
      if (ScrSize.first <= 0 || ScrSize.second <= 0)
      {
        Silent = true;
        return;
      }
      const int_t trackingHeight = moduleState.Track ? TRACKING_HEIGHT : 0;
      const int_t bodyHeight = ScrSize.second - INFORMATION_HEIGHT - trackingHeight - PLAYING_HEIGHT - 1;
      if (ScrSize.second < int_t(trackingHeight + PLAYING_HEIGHT))
      {
        Quiet = true;
        return;
      }
      Vsync();
      if (moduleState.Track)
      {
        ShowTrackingStatus(*moduleState.Track);
      }
      ShowPlaybackStatus(Time::Milliseconds(moduleState.At.CastTo<Time::Millisecond>().Get()), playbackState);
      bool showPattern = false;
      if (TrackingView && moduleState.Track && bodyHeight >= 1)
      {
        if (TrackingView->Update(*moduleState.Track, {ScrSize.first, bodyHeight}))
        {
          showPattern = true;
        }
        else
        {
          TrackingView.reset();
        }
      }
      if (showPattern)
      {
        DynamicLines += TrackingView->Draw();
      }
      else if (Analyzer && bodyHeight >= 4)  // minimal spectrum height
      {
        Sound::Analyzer::LevelType spectrum[ScrSize.first];
        Analyzer->GetSpectrum(spectrum, ScrSize.first);
        AnalyzerData.resize(ScrSize.first);
        UpdateAnalyzer(spectrum);
        ShowAnalyzer(bodyHeight);
      }
      else if (Analyzer)
      {
        Analyzer.reset();
      }
      StdOut << std::flush;
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
      StdOut << Strings::Format(TRACKING_FORMAT, state.Position, state.Pattern, state.Line, state.Quirk, state.Channels,
                                state.Tempo);
      DynamicLines += TRACKING_HEIGHT;
    }

    void ShowPlaybackStatus(Time::Milliseconds played, Sound::PlaybackControl::PlaybackState state)
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

    static char StateSymbol(Sound::PlaybackControl::PlaybackState state)
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
    String AnalyzerMode;
    uint_t Updatefps = 10;
    const Strings::Template::Ptr InformationTemplate;
    // context
    std::chrono::time_point<std::chrono::steady_clock> NextFrameStart;
    std::size_t DynamicLines = 0;
    Console::SizeType ScrSize;
    Time::Milliseconds TotalDuration;
    Sound::Analyzer::Ptr Analyzer;
    std::vector<int_t> AnalyzerData;
    std::optional<PatternView> TrackingView;
  };
}  // namespace

DisplayComponent::Ptr DisplayComponent::Create()
{
  return DisplayComponent::Ptr(new DisplayComponentImpl);
}
