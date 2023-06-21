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
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>

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

  private:
    void InitState(Sound::Backend::Ptr player, Playlist::Item::Data::Ptr)
    {
      TrackState = std::dynamic_pointer_cast<const Module::TrackState>(player->GetState());
      CloseState();
    }

    void UpdateState()
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

    void CloseState()
    {
      textPosition->setText(EMPTY_TEXT);
      textPattern->setText(EMPTY_TEXT);
      textLine->setText(EMPTY_TEXT);
      textFrame->setText(EMPTY_TEXT);
      textChannels->setText(EMPTY_TEXT);
      textTempo->setText(EMPTY_TEXT);
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
