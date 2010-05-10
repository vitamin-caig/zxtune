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
#include "playback_controls_ui.h"
#include "playback_controls_moc.h"
//std includes
#include <cassert>
//qt includes
#include <QtGui/QMainWindow>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>

namespace
{
  class PlaybackControlsImpl : public PlaybackControls
                             , private Ui::PlaybackControls
  {
  public:
    explicit PlaybackControlsImpl(QMainWindow* parent)
    {
      setParent(parent);
      setupUi(this);
      //create and fill menu
      QMenuBar* const menuBar = parent->menuBar();
      QMenu* const menu = menuBar->addMenu(QString::fromUtf8("Playback"));
      menu->addAction(actionPlay);
      menu->addAction(actionPause);
      menu->addAction(actionStop);
      menu->addSeparator();
      menu->addAction(actionPrevious);
      menu->addAction(actionNext);
      
      //connect actions with self signals
      connect(actionPlay, SIGNAL(triggered()), SIGNAL(OnPlay()));
      connect(actionPause, SIGNAL(triggered()), SIGNAL(OnPause()));
      connect(actionStop, SIGNAL(triggered()), SIGNAL(OnStop()));
      connect(actionPrevious, SIGNAL(triggered()), SIGNAL(OnPrevious()));
      connect(actionNext, SIGNAL(triggered()), SIGNAL(OnNext()));
    }
  };
}

PlaybackControls* PlaybackControls::Create(QMainWindow* parent)
{
  assert(parent);
  return new PlaybackControlsImpl(parent);
}
