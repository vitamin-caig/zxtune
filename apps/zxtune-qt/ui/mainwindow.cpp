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
#include "mainwindow_ui.h"
#include "mainwindow_moc.h"
#include "playback_controls.h"
#include "seek_controls.h"
#include "playlist.h"
#include <apps/base/moduleitem.h>
//common includes
#include <logging.h>

namespace
{
  class MainWindowImpl : public MainWindow
                       , private Ui::MainWindow
  {
  public:
    MainWindowImpl(int argc, char* argv[])
      : Controls(new PlaybackControls(this))
      , Seeking(new SeekControls(this))
    {
      //fill and layout
      setupUi(this);
      //add widgets to toolbars
      controlToolbar->addWidget(Controls);
      seekToolbar->addWidget(Seeking);
      Playlist* const playlist = Playlist::Create(this);
      //TODO: load from config
      playlistsContainer->addTab(playlist, QString::fromUtf8("Default playlist"));

      //TODO: remove
      for (int param = 1; param < argc; ++param)
      {
        playlist->AddItemByPath(FromStdString(argv[param]));
      }
    }
  private:
    PlaybackControls* const Controls;
    SeekControls* const Seeking;
  };
}

QPointer<MainWindow> MainWindow::Create(int argc, char* argv[])
{
  //register metatypes
  qRegisterMetaType<Log::MessageData>("Log::MessageData");
  qRegisterMetaType<ModuleItem>("ModuleItem");
  return QPointer<MainWindow>(new MainWindowImpl(argc, argv));
}
