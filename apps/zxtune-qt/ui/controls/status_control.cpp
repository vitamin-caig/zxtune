/**
 *
 * @file
 *
 * @brief Status control widget implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "status_control.h"
#include "status_control.ui.h"
#include "supp/playback_supp.h"
#include "ui/utils.h"
// common includes
#include <contract.h>
// library includes
#include <module/track_state.h>
// std includes
#include <utility>
// qt includes
#include <QtGui/QGridLayout>
#include <QtGui/QLabel>

namespace
{
  const QString EMPTY_TEXT(QLatin1String("-"));

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

      Require(connect(&supp, SIGNAL(OnStartModule(Sound::Backend::Ptr, Playlist::Item::Data::Ptr)),
                      SLOT(InitState(Sound::Backend::Ptr))));
      Require(connect(&supp, SIGNAL(OnUpdateState()), SLOT(UpdateState())));
      Require(connect(&supp, SIGNAL(OnStopModule()), SLOT(CloseState())));
    }

    void InitState(Sound::Backend::Ptr player) override
    {
      TrackState = std::dynamic_pointer_cast<const Module::TrackState>(player->GetState());
      CloseState();
    }

    void UpdateState() override
    {
      if (isVisible() && TrackState)
      {
        textPosition->setText(QString::number(TrackState->Position()));
        textPattern->setText(QString::number(TrackState->Pattern()));
        textLine->setText(QString::number(TrackState->Line()));
        textFrame->setText(QString::number(TrackState->Quirk()));
        textChannels->setText(QString::number(TrackState->Channels()));
        textTempo->setText(QString::number(TrackState->Tempo()));
      }
    }

    void CloseState() override
    {
      textPosition->setText(EMPTY_TEXT);
      textPattern->setText(EMPTY_TEXT);
      textLine->setText(EMPTY_TEXT);
      textFrame->setText(EMPTY_TEXT);
      textChannels->setText(EMPTY_TEXT);
      textTempo->setText(EMPTY_TEXT);
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

  private:
    Module::TrackState::Ptr TrackState;
  };
}  // namespace

StatusControl::StatusControl(QWidget& parent)
  : QWidget(&parent)
{}

StatusControl* StatusControl::Create(QWidget& parent, PlaybackSupport& supp)
{
  return new StatusControlImpl(parent, supp);
}
