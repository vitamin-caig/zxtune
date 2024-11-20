/**
 *
 * @file
 *
 * @brief Seek controls widget implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune-qt/ui/controls/seek_controls.h"

#include "apps/zxtune-qt/supp/playback_supp.h"
#include "apps/zxtune-qt/ui/styles.h"
#include "apps/zxtune-qt/ui/utils.h"
#include "seek_controls.ui.h"

#include "time/serialize.h"

#include "contract.h"

#include <QtGui/QCursor>
#include <QtWidgets/QToolTip>

#include <memory>

namespace
{
  const auto PRECISION = Time::Milliseconds(100);

  // Time/Frames/Slider logic
  class ScaledTime
  {
  public:
    ScaledTime(const Playlist::Item::Data& data, Module::State::Ptr state)
      : Duration(data.GetDuration())
      , State(std::move(state))
    {}

    int GetSliderRange() const
    {
      return Duration.Divide<int>(PRECISION);
    }

    int GetSliderPosition() const
    {
      return GetPlayed().Divide<int>(PRECISION);
    }

    Time::AtMillisecond SliderPositionToSeekPosition(int pos) const
    {
      return Time::AtMillisecond() + SliderPositionToTime(pos);
    }

    static Time::Milliseconds SliderPositionToTime(int pos)
    {
      return PRECISION * pos;
    }

    Time::Milliseconds GetPlayed() const
    {
      return Time::Milliseconds{State->At().CastTo<Time::Millisecond>().Get()};
    }

  private:
    const Time::Milliseconds Duration;
    const Module::State::Ptr State;
  };

  class SeekControlsImpl
    : public SeekControls
    , private Ui::SeekControls
  {
  public:
    SeekControlsImpl(QWidget& parent, PlaybackSupport& supp)
      : ::SeekControls(parent)
    {
      // setup self
      setupUi(this);
      timePosition->setRange(0, 0);
      Require(connect(timePosition, &QSlider::sliderReleased, this, &SeekControlsImpl::EndSeeking));

      Require(connect(&supp, &PlaybackSupport::OnStartModule, this, &SeekControlsImpl::InitState));
      Require(connect(&supp, &PlaybackSupport::OnUpdateState, this, &SeekControlsImpl::UpdateState));
      Require(connect(&supp, &PlaybackSupport::OnStopModule, this, &SeekControlsImpl::CloseState));
      Require(connect(this, &::SeekControls::OnSeeking, &supp, &PlaybackSupport::Seek));
      timePosition->setStyle(UI::GetStyle());
    }

    // QWidget
    void changeEvent(QEvent* event) override
    {
      if (event && QEvent::LanguageChange == event->type())
      {
        retranslateUi(this);
      }
      ::SeekControls::changeEvent(event);
    }

  private:
    void InitState(Sound::Backend::Ptr player, Playlist::Item::Data::Ptr item)
    {
      Scaler = std::make_unique<ScaledTime>(*item, player->GetState());
      timePosition->setRange(0, Scaler->GetSliderRange());
    }

    void UpdateState()
    {
      if (!isVisible())
      {
        return;
      }
      if (timePosition->isSliderDown())
      {
        ShowTooltip();
      }
      else
      {
        timePosition->setSliderPosition(Scaler->GetSliderPosition());
      }
      timeDisplay->setText(ToQString(Time::ToString(Scaler->GetPlayed())));
    }

    void CloseState()
    {
      timePosition->setRange(0, 0);
      timeDisplay->setText(QLatin1String("-:-.-"));
    }

    void EndSeeking()
    {
      OnSeeking(Scaler->SliderPositionToSeekPosition(timePosition->value()));
    }

    void ShowTooltip()
    {
      const uint_t sliderPos = timePosition->value();
      const QString text = ToQString(Time::ToString(Scaler->SliderPositionToTime(sliderPos)));
      const QPoint& mousePos = QCursor::pos();
      QToolTip::showText(mousePos, text, timePosition, QRect());
    }

  private:
    std::unique_ptr<ScaledTime> Scaler;
  };
}  // namespace

SeekControls::SeekControls(QWidget& parent)
  : QWidget(&parent)
{}

SeekControls* SeekControls::Create(QWidget& parent, PlaybackSupport& supp)
{
  return new SeekControlsImpl(parent, supp);
}
