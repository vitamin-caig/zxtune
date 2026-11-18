/**
 *
 * @file
 *
 * @brief Status control widget implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune-qt/ui/controls/status_control.h"

#include "apps/zxtune-qt/supp/playback_supp.h"
#include "apps/zxtune-qt/supp/thread_utils.h"
#include "apps/zxtune-qt/ui/utils.h"
#include "status_control.ui.h"

#include "module/state.h"
#include "module/track_presentation.h"
#include "strings/split.h"

#include "contract.h"
#include "string_view.h"

#include <QtCore/QEvent>
#include <QtGui/QColor>
#include <QtGui/QFontDatabase>
#include <QtGui/QHelpEvent>
#include <QtGui/QHideEvent>
#include <QtWidgets/QGraphicsDropShadowEffect>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>

#include <algorithm>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace
{
  const QString EMPTY_TEXT = "-"_latin;
  // Keep the tooltip compact; the current line is centered inside this window
  constexpr int TOOLTIP_ROWS = 15;
  constexpr int TOOLTIP_COLUMNS = 120;
  // Reserve room for the label's drop shadow; otherwise Qt clips it at the window edge
  constexpr int TOOLTIP_SHADOW_MARGIN = 16;
  constexpr auto SELECTION_STYLE = "background-color:palette(highlight); color:palette(highlighted-text);"sv;

  class TooltipWidget : public QWidget
  {
  public:
    TooltipWidget()
      : QWidget(nullptr, Qt::ToolTip)
    {
      setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint);
      setAttribute(Qt::WA_ShowWithoutActivating);
      setAttribute(Qt::WA_TransparentForMouseEvents);
      setAttribute(Qt::WA_TranslucentBackground);

      auto* layout = new QVBoxLayout(this);
      layout->setContentsMargins(TOOLTIP_SHADOW_MARGIN, TOOLTIP_SHADOW_MARGIN, TOOLTIP_SHADOW_MARGIN,
                                 TOOLTIP_SHADOW_MARGIN);
      layout->setSpacing(0);

      Label = new QLabel(this);
      Label->setTextFormat(Qt::RichText);
      Label->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
      Label->setMargin(4);
      Label->setAutoFillBackground(true);
      Label->setAttribute(Qt::WA_TransparentForMouseEvents);
      layout->addWidget(Label);

      auto* shadow = new QGraphicsDropShadowEffect;
      shadow->setBlurRadius(12);
      shadow->setOffset(2, 4);
      shadow->setColor(QColor(0, 0, 0, 160));
      Label->setGraphicsEffect(shadow);
    }

    void SetText(const QString& text)
    {
      Label->setText(text);
      adjustSize();
    }

  private:
    QLabel* Label = nullptr;
  };

  //! Pattern content requested on the IO thread and rendered on the UI thread
  class PatternModel
  {
  public:
    PatternModel(Module::TrackPresentation::Ptr source, uint_t index)
      : Source(std::move(source))
      , Index(index)
      , Content(Source->GetPattern(index))
      , Lines(Strings::Split(Content, '\n'))
    {}

    PatternModel(const PatternModel&) = delete;
    PatternModel& operator=(const PatternModel&) = delete;

    //! @return empty string if the state does not change the visible content
    String GetUpdatedContent(const Module::State& newState)
    {
      if (!newState.Track || newState.Track->Pattern != Index || Content.empty())
      {
        return {};
      }
      const auto line = newState.Track->Line;
      if (LastLine && *LastLine == line)
      {
        return {};
      }
      LastLine = line;
      return Render(line);
    }

    bool Matches(const Module::TrackPresentation& source, uint_t index) const
    {
      return Source.get() == &source && Index == index;
    }

    bool Empty() const
    {
      return Content.empty();
    }

  private:
    String Render(uint_t line) const
    {
      const int total = static_cast<int>(Lines.size());
      if (total <= 0)
      {
        return {};
      }
      const int current = static_cast<int>(line);
      constexpr int CENTER = TOOLTIP_ROWS / 2;

      String result{"<html><span style=\"white-space:pre\">"sv};
      for (int offset = 0; offset != TOOLTIP_ROWS; ++offset)
      {
        if (offset != 0)
        {
          result += "<br/>"sv;
        }
        const int row = current + offset - CENTER;
        if (row < 0 || row >= total)
        {
          continue;
        }
        const auto text = Lines[row].substr(0, TOOLTIP_COLUMNS);
        if (row == current)
        {
          result += "<span style=\""sv;
          result += SELECTION_STYLE;
          result += "\">"sv;
          result += text;
          result += "</span>"sv;
        }
        else
        {
          result += text;
        }
      }
      result += "</span></html>"sv;
      return result;
    }

  private:
    const Module::TrackPresentation::Ptr Source;
    const uint_t Index = 0;
    const String Content;
    const std::vector<StringView> Lines;
    std::optional<uint_t> LastLine;
  };

  class StatusControlImpl
    : public StatusControl
    , private Ui::StatusControl
  {
  public:
    StatusControlImpl(QWidget& parent, PlaybackSupport& supp)
      : ::StatusControl(parent)
    {
      // setup self
      setupUi(this);

      // The tooltip is handled by the whole control, not by the individual labels
      for (auto* widget : findChildren<QWidget*>())
      {
        widget->setAttribute(Qt::WA_TransparentForMouseEvents);
      }

      Require(connect(&supp, &PlaybackSupport::OnStartModule, this, &StatusControlImpl::InitState));
      Require(connect(&supp, &PlaybackSupport::OnUpdateState, this, &StatusControlImpl::UpdateState));
      Require(connect(&supp, &PlaybackSupport::OnStopModule, this, &StatusControlImpl::CloseState));
    }

    // QWidget
    void changeEvent(QEvent* event) override
    {
      if (event && QEvent::LanguageChange == event->type())
      {
        retranslateUi(this);
      }
      ::StatusControl::changeEvent(event);
    }

    bool event(QEvent* event) override
    {
      if (event && QEvent::ToolTip == event->type())
      {
        ShowTooltip(static_cast<QHelpEvent*>(event)->globalPos());
        return true;
      }
      return ::StatusControl::event(event);
    }

    void leaveEvent(QEvent* event) override
    {
      HideTooltip();
      ::StatusControl::leaveEvent(event);
    }

    void hideEvent(QHideEvent* event) override
    {
      HideTooltip();
      ::StatusControl::hideEvent(event);
    }

  private:
    void InitState(Sound::Backend::Ptr player, Playlist::Item::Data::Ptr, Module::Holder::Ptr module)
    {
      Control = player->GetPlaybackControl();
      CloseState();
      // The presentation keeps the concrete holder alive while pattern is rendered on the IO thread
      Track = std::dynamic_pointer_cast<const Module::TrackPresentation>(module);
      if (ActiveTooltipPosition && Control)
      {
        RefreshTooltip(Control->GetModuleState());
      }
    }

    void UpdateState()
    {
      if (!isVisible())
      {
        return;
      }
      const auto& state = Control->GetModuleState();
      if (state.Track)
      {
        const auto& track = *state.Track;
        textPosition->setText(QString::number(track.Position));
        textPattern->setText(QString::number(track.Pattern));
        textLine->setText(QString::number(track.Line));
        textFrame->setText(QString::number(track.Quirk));
        textChannels->setText(QString::number(track.Channels));
        textTempo->setText(QString::number(track.Tempo));
      }
      if (ActiveTooltipPosition)
      {
        RefreshTooltip(state);
      }
    }

    void CloseState()
    {
      Request.reset();
      Cache.reset();
      // Keep the hover state across stop/start sequences used for track switching
      DismissTooltip();
      Track.reset();
      textPosition->setText(EMPTY_TEXT);
      textPattern->setText(EMPTY_TEXT);
      textLine->setText(EMPTY_TEXT);
      textFrame->setText(EMPTY_TEXT);
      textChannels->setText(EMPTY_TEXT);
      textTempo->setText(EMPTY_TEXT);
    }

    void ShowTooltip(const QPoint& pos)
    {
      if (Control)
      {
        ActiveTooltipPosition = pos;
        RefreshTooltip(Control->GetModuleState());
      }
    }

    void HideTooltip()
    {
      ActiveTooltipPosition.reset();
      DismissTooltip();
    }

    void DismissTooltip()
    {
      if (Tooltip)
      {
        Tooltip->hide();
      }
    }

    //! Starts loading if needed; returns true when the cache already matches the pattern
    bool EnsurePattern(uint_t pattern)
    {
      if (!Track)
      {
        return false;
      }
      if (Cache && Cache->Matches(*Track, pattern))
      {
        return true;
      }
      if (!Request)
      {
        Request = pattern;
        IOThread::Execute(this, &StatusControlImpl::LoadPattern, Track, pattern);
      }
      return false;
    }

    static void LoadPattern(QPointer<StatusControlImpl> self, Module::TrackPresentation::Ptr presentation,
                            uint_t pattern)
    {
      auto model = std::make_unique<PatternModel>(std::move(presentation), pattern);
      SelfThread::Execute(self, &StatusControlImpl::ApplyPattern, std::move(model));
    }

    void ApplyPattern(std::unique_ptr<PatternModel> loaded)
    {
      if (!loaded || !Track || !Request || !loaded->Matches(*Track, *Request))
      {
        return;
      }
      Request.reset();
      Cache = std::move(loaded);
      if (ActiveTooltipPosition && Control)
      {
        RefreshTooltip(Control->GetModuleState());
      }
    }

    void RefreshTooltip(const Module::State& state)
    {
      if (!ActiveTooltipPosition)
      {
        return;
      }
      if (!state.Track || !Track || !EnsurePattern(state.Track->Pattern) || Cache->Empty())
      {
        if (Tooltip)
        {
          Tooltip->hide();
        }
        return;
      }
      if (!Tooltip)
      {
        Tooltip = std::make_unique<TooltipWidget>();
      }
      if (const String html = Cache->GetUpdatedContent(state); !html.empty())
      {
        Tooltip->SetText(ToQString(html));
      }
      Tooltip->move(*ActiveTooltipPosition + QPoint(16 - TOOLTIP_SHADOW_MARGIN, 16 - TOOLTIP_SHADOW_MARGIN));
      Tooltip->show();
      Tooltip->raise();
    }

  private:
    Sound::PlaybackControl::Ptr Control;
    Module::TrackPresentation::Ptr Track;
    std::optional<uint_t> Request;
    std::unique_ptr<PatternModel> Cache;
    std::optional<QPoint> ActiveTooltipPosition;
    std::unique_ptr<TooltipWidget> Tooltip;
  };
}  // namespace

StatusControl::StatusControl(QWidget& parent)
  : QWidget(&parent)
{}

StatusControl* StatusControl::Create(QWidget& parent, PlaybackSupport& supp)
{
  return new StatusControlImpl(parent, supp);
}
