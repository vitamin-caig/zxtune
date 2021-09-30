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
#include "playback_controls.h"
#include "playback_controls.ui.h"
#include "supp/playback_supp.h"
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
      Require(connect(actionPlay, SIGNAL(triggered()), SIGNAL(OnPlay())));
      Require(connect(actionPause, SIGNAL(triggered()), SIGNAL(OnPause())));
      Require(connect(actionStop, SIGNAL(triggered()), SIGNAL(OnStop())));
      Require(connect(actionPrevious, SIGNAL(triggered()), SIGNAL(OnPrevious())));
      Require(connect(actionNext, SIGNAL(triggered()), SIGNAL(OnNext())));

      Require(supp.connect(this, SIGNAL(OnPlay()), SLOT(Play())));
      Require(supp.connect(this, SIGNAL(OnStop()), SLOT(Stop())));
      Require(supp.connect(this, SIGNAL(OnPause()), SLOT(Pause())));
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
