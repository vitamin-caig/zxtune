/**
 *
 * @file
 *
 * @brief Playback controls widget implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "apps/zxtune-qt/ui/controls/playback_controls.h"
#include "apps/zxtune-qt/supp/playback_supp.h"
#include "playback_controls.ui.h"
// common includes
#include <contract.h>
// std includes
#include <cassert>
#include <utility>
// qt includes
#include <QtWidgets/QMenu>

namespace
{
  class PlaybackControlsImpl
    : public PlaybackControls
    , private Ui::PlaybackControls
  {
  public:
    PlaybackControlsImpl(QWidget& parent, PlaybackSupport& supp)
      : ::PlaybackControls(parent)
      , ActionsMenu(new QMenu(this))
    {
      // setup self
      setupUi(this);
      SetupMenu();

      // connect actions with self signals
      Require(connect(actionPlay, &QAction::triggered, this, &::PlaybackControls::OnPlay));
      Require(connect(actionPause, &QAction::triggered, this, &::PlaybackControls::OnPause));
      Require(connect(actionStop, &QAction::triggered, this, &::PlaybackControls::OnStop));
      Require(connect(actionPrevious, &QAction::triggered, this, &::PlaybackControls::OnPrevious));
      Require(connect(actionNext, &QAction::triggered, this, &::PlaybackControls::OnNext));

      Require(connect(this, &::PlaybackControls::OnPlay, &supp, &PlaybackSupport::Play));
      Require(connect(this, &::PlaybackControls::OnStop, &supp, &PlaybackSupport::Stop));
      Require(connect(this, &::PlaybackControls::OnPause, &supp, &PlaybackSupport::Pause));
    }

    QMenu* GetActionsMenu() const override
    {
      return ActionsMenu;
    }

    // QWidget
    void changeEvent(QEvent* event) override
    {
      if (event && QEvent::LanguageChange == event->type())
      {
        retranslateUi(this);
        SetMenuTitle();
      }
      ::PlaybackControls::changeEvent(event);
    }

  private:
    void SetupMenu()
    {
      SetMenuTitle();
      ActionsMenu->addAction(actionPlay);
      ActionsMenu->addAction(actionPause);
      ActionsMenu->addAction(actionStop);
      ActionsMenu->addSeparator();
      ActionsMenu->addAction(actionPrevious);
      ActionsMenu->addAction(actionNext);
    }

    void SetMenuTitle()
    {
      ActionsMenu->setTitle(::PlaybackControls::tr("Playback"));
    }

  private:
    QMenu* const ActionsMenu;
  };
}  // namespace

PlaybackControls::PlaybackControls(QWidget& parent)
  : QWidget(&parent)
{}

PlaybackControls* PlaybackControls::Create(QWidget& parent, PlaybackSupport& supp)
{
  return new PlaybackControlsImpl(parent, supp);
}
