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
#include "playback_thread.h"
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
      , Thread(PlaybackThread::Create(this))
    {
      //fill and layout
      setupUi(this);
      //add widgets to toolbars
      controlToolbar->addWidget(Controls);
      seekToolbar->addWidget(Seeking);
      CurrentPlaylist = Playlist::Create(this);
      //TODO: load from config
      playlistsContainer->addTab(CurrentPlaylist, QString::fromUtf8("Default playlist"));

      //TODO: remove
      for (int param = 1; param < argc; ++param)
      {
        CurrentPlaylist->AddItemByPath(FromStdString(argv[param]));
      }

      //connect actions
      Thread->connect(CurrentPlaylist, SIGNAL(OnItemSelected(const ModuleItem&)), SLOT(SetItem(const ModuleItem&)));
      Thread->connect(Controls->playButton, SIGNAL(clicked(bool)), SLOT(Play()));
      Thread->connect(Controls->stopButton, SIGNAL(clicked(bool)), SLOT(Stop()));
      Thread->connect(Controls->pauseButton, SIGNAL(clicked(bool)), SLOT(Pause()));
    }
  private:
    PlaybackControls* const Controls;
    SeekControls* const Seeking;
    PlaybackThread* const Thread;
    Playlist* CurrentPlaylist;
  };
}

QPointer<MainWindow> MainWindow::Create(int argc, char* argv[])
{
  //register metatypes
  //TODO: extract to function
  qRegisterMetaType<Log::MessageData>("Log::MessageData");
  qRegisterMetaType<ModuleItem>("ModuleItem");
  qRegisterMetaType<ZXTune::Module::Tracking>("ZXTune::Module::Tracking");
  qRegisterMetaType<ZXTune::Module::Analyze::ChannelsState>("ZXTune::Module::Analyze::ChannelsState");
  return QPointer<MainWindow>(new MainWindowImpl(argc, argv));
}
