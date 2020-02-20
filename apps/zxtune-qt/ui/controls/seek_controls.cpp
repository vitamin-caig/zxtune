/**
* 
* @file
*
* @brief Seek controls widget implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "seek_controls.h"
#include "seek_controls.ui.h"
#include "supp/playback_supp.h"
#include "ui/styles.h"
#include "ui/utils.h"
//common includes
#include <contract.h>
//library includes
#include <time/serialize.h>
//qt includes
#include <QtGui/QCursor>
#include <QtGui/QToolTip>

namespace
{
  // Time/Frames/Slider logic
  class ScaledTime
  {
  public:
    ScaledTime(const Playlist::Item::Data& data, Module::State::Ptr state)
      : Frames(data.GetModule()->GetModuleInformation()->FramesCount())
      , Duration(data.GetDuration())
      , State(std::move(state))
    {
    }

    int GetSliderRange() const
    {
      return Frames;
    }

    int GetSliderPosition() const
    {
      return State->Frame();
    }

    uint_t SliderPositionToSeekPosition(int pos) const
    {
      return pos;
    }

    Time::Milliseconds SliderPositionToTime(int pos) const
    {
      const uint64_t frame = pos;
      return decltype(Duration)(frame * Duration.Get() / Frames);
    }

    Time::Milliseconds GetPosition() const
    {
      return SliderPositionToTime(State->Frame());
    }
  private:
    const uint_t Frames;
    const Time::Milliseconds Duration;
    const Module::State::Ptr State;
  };

  class SeekControlsImpl : public SeekControls
                         , private Ui::SeekControls
  {
  public:
    SeekControlsImpl(QWidget& parent, PlaybackSupport& supp)
      : ::SeekControls(parent)
    {
      //setup self
      setupUi(this);
      timePosition->setRange(0, 0);
      Require(connect(timePosition, SIGNAL(sliderReleased()), SLOT(EndSeeking())));

      Require(connect(&supp, SIGNAL(OnStartModule(Sound::Backend::Ptr, Playlist::Item::Data::Ptr)),
        SLOT(InitState(Sound::Backend::Ptr, Playlist::Item::Data::Ptr))));
      Require(connect(&supp, SIGNAL(OnUpdateState()), SLOT(UpdateState())));
      Require(connect(&supp, SIGNAL(OnStopModule()), SLOT(CloseState())));
      Require(supp.connect(this, SIGNAL(OnSeeking(int)), SLOT(Seek(int))));
      timePosition->setStyle(UI::GetStyle());
    }

    void InitState(Sound::Backend::Ptr player, Playlist::Item::Data::Ptr item) override
    {
      Scaler.reset(new ScaledTime(*item, player->GetState()));
      timePosition->setRange(0, Scaler->GetSliderRange());
    }

    void UpdateState() override
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
      timeDisplay->setText(ToQString(Time::ToString(Scaler->GetPosition())));
    }

    void CloseState() override
    {
      timePosition->setRange(0, 0);
      timeDisplay->setText(QLatin1String("-:-.-"));
    }

    void EndSeeking() override
    {
      OnSeeking(Scaler->SliderPositionToSeekPosition(timePosition->value()));
    }

    //QWidget
    void changeEvent(QEvent* event) override
    {
      if (event && QEvent::LanguageChange == event->type())
      {
        retranslateUi(this);
      }
      ::SeekControls::changeEvent(event);
    }
  private:
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
}

SeekControls::SeekControls(QWidget& parent) : QWidget(&parent)
{
}

SeekControls* SeekControls::Create(QWidget& parent, PlaybackSupport& supp)
{
  return new SeekControlsImpl(parent, supp);
}
