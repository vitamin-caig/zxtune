/*
Abstract:
  Playback controller creation implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "playback_controls.h"
#include "playback_controls.ui.h"
//std includes
#include <cassert>
//qt includes
#include <QtGui/QMenu>

namespace
{
  class PlaybackControlsImpl : public PlaybackControls
                             , private Ui::PlaybackControls
  {
  public:
    explicit PlaybackControlsImpl(QWidget& parent)
      : ::PlaybackControls(parent)
      , ActionsMenu(new QMenu(tr("Playback"), this))
    {
      //setup self
      setupUi(this);
      SetupMenu();

      //connect actions with self signals
      connect(actionPlay, SIGNAL(triggered()), SIGNAL(OnPlay()));
      connect(actionPause, SIGNAL(triggered()), SIGNAL(OnPause()));
      connect(actionStop, SIGNAL(triggered()), SIGNAL(OnStop()));
      connect(actionPrevious, SIGNAL(triggered()), SIGNAL(OnPrevious()));
      connect(actionNext, SIGNAL(triggered()), SIGNAL(OnNext()));
    }

    virtual QMenu* GetActionsMenu() const
    {
      return ActionsMenu;
    }
  private:
    void SetupMenu()
    {
      ActionsMenu->addAction(actionPlay);
      ActionsMenu->addAction(actionPause);
      ActionsMenu->addAction(actionStop);
      ActionsMenu->addSeparator();
      ActionsMenu->addAction(actionPrevious);
      ActionsMenu->addAction(actionNext);
    }
  private:
    QMenu* const ActionsMenu;
  };
}

PlaybackControls::PlaybackControls(QWidget& parent) : QWidget(&parent)
{
}

PlaybackControls* PlaybackControls::Create(QWidget& parent)
{
  return new PlaybackControlsImpl(parent);
}
