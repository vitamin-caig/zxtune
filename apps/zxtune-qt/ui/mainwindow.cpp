/*
Abstract:
  Main window implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "mainwindow.h"
#include "mainwindow_moc.h"
#include "playback_controls.h"
#include "seek_controls.h"
#include "playlist.h"


MainWindow::MainWindow(int argc, char* argv[])
  : Controls(new PlaybackControls(this))
  , Seeking(new SeekControls(this))
{
    //fill and layout
    setupUi(this);
    //add widgets to toolbars
    controlToolbar->addWidget(Controls);
    seekToolbar->addWidget(Seeking);
    Playlist* const playlist = Playlist::Create(this);
    playlistsContainer->addTab(playlist, QString::fromUtf8("Default playlist"));
    
    //TODO: remove
    for (int param = 1; param < argc; ++param)
    {
      playlist->AddItemByPath(FromStdString(argv[param]));
    }
}
