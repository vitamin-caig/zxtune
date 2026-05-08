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
#include "apps/zxtune-qt/ui/utils.h"
#include "status_control.ui.h"

#include "contract.h"

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>

#include <utility>

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
      Control = player->GetPlaybackControl();
      CloseState();
    }

    void UpdateState()
    {
      if (!isVisible())
      {
        return;
      }
      if (const auto& state = Control->GetModuleState(); state.Track)
      {
        const auto& track = *state.Track;
        textPosition->setText(QString::number(track.Position));
        textPattern->setText(QString::number(track.Pattern));
        textLine->setText(QString::number(track.Line));
        textFrame->setText(QString::number(track.Quirk));
        textChannels->setText(QString::number(track.Channels));
        textTempo->setText(QString::number(track.Tempo));
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
    Sound::PlaybackControl::Ptr Control;
  };
}  // namespace

StatusControl::StatusControl(QWidget& parent)
  : QWidget(&parent)
{}

StatusControl* StatusControl::Create(QWidget& parent, PlaybackSupport& supp)
{
  return new StatusControlImpl(parent, supp);
}
