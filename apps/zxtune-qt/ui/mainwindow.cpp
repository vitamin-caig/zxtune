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
#include "controls/playback_controls.h"
#include "controls/seek_controls.h"
#include "playlist/playlist.h"
#include "playback_thread.h"
#include <apps/base/moduleitem.h>
//common includes
#include <logging.h>
//qt includes
#include <QtGui/QToolBar>

namespace
{
  template<class T>
  class ControlOnToolbar
  {
  public:
    ControlOnToolbar(QMainWindow* mainWindow, QMenu* layoutMenu, const char* menuTitle)
      : Control(T::Create(mainWindow))
      , Toolbar(new QToolBar(mainWindow))
      , Action(new QAction(mainWindow))
    {
      //setup toolbar
      QSizePolicy sizePolicy1(QSizePolicy::Maximum, QSizePolicy::MinimumExpanding);
      sizePolicy1.setHorizontalStretch(0);
      sizePolicy1.setVerticalStretch(0);
      sizePolicy1.setHeightForWidth(Toolbar->sizePolicy().hasHeightForWidth());
      Toolbar->setSizePolicy(sizePolicy1);
      Toolbar->setAllowedAreas(Qt::TopToolBarArea);
      Toolbar->setFloatable(false);
      mainWindow->addToolBar(Qt::TopToolBarArea, Toolbar);
      Toolbar->addWidget(Control);
      //setup action
      Action->setCheckable(true);
      Action->setChecked(true);//TODO
      Action->setText(QApplication::translate(mainWindow->objectName().toStdString().c_str(), menuTitle, 0, QApplication::UnicodeUTF8));
      //integrate
      layoutMenu->addAction(Action);
      Toolbar->connect(Action, SIGNAL(toggled(bool)), SLOT(setVisible(bool)));
    }

    //accessors
    T* operator -> () const
    {
      return Control;
    }

    operator T* () const
    {
      return Control;
    }
  private:
    T* const Control;
    QToolBar* const Toolbar;
    QAction* const Action;
  };
  
  class UiHelper : public Ui::MainWindow
  {
  public:
    explicit UiHelper(QMainWindow* mainWindow)
    {
      setupUi(mainWindow);
    }
  };

  class MainWindowImpl : public MainWindow
                       , private UiHelper
  {
  public:
    MainWindowImpl(int argc, char* argv[])
      : UiHelper(this)
      , Controls(this, menuLayout, "Controls")
      , Seeking(this, menuLayout, "Seeking")
      , Thread(PlaybackThread::Create(this))
    {
      //setup playlist
      CurrentPlaylist = Playlist::Create(this);
      //TODO: load from config
      playlistsContainer->addTab(CurrentPlaylist, QString::fromUtf8("Default playlist"));

      //TODO: remove
      for (int param = 1; param < argc; ++param)
      {
        CurrentPlaylist->AddItemByPath(FromStdString(argv[param]));
      }

      //connect root actions
      CurrentPlaylist->connect(actionPrevious, SIGNAL(triggered(bool)), SLOT(PrevItem()));
      CurrentPlaylist->connect(actionNext, SIGNAL(triggered(bool)), SLOT(NextItem()));
      CurrentPlaylist->connect(Thread, SIGNAL(OnFinishModule(const ZXTune::Module::Information&)), SLOT(NextItem()));
      Thread->connect(CurrentPlaylist, SIGNAL(OnItemSelected(const ModuleItem&)), SLOT(SetItem(const ModuleItem&)));
      Thread->connect(actionPlay, SIGNAL(triggered(bool)), SLOT(Play()));
      Thread->connect(actionStop, SIGNAL(triggered(bool)), SLOT(Stop()));
      Thread->connect(actionPause, SIGNAL(triggered(bool)), SLOT(Pause()));
      Thread->connect(Seeking, SIGNAL(OnSeeking(int)), SLOT(Seek(int)));
      Seeking->connect(Thread, SIGNAL(OnStartModule(const ZXTune::Module::Information&)), SLOT(InitState(const ZXTune::Module::Information&)));
      Seeking->connect(Thread, SIGNAL(OnUpdateState(uint, const ZXTune::Module::Tracking&, const ZXTune::Module::Analyze::ChannelsState&)),
        SLOT(UpdateState(uint)));
      Seeking->connect(Thread, SIGNAL(OnStopModule(const ZXTune::Module::Information&)), SLOT(CloseState(const ZXTune::Module::Information&)));
      //distrubute actions to controls
      actionPrevious->connect(Controls->prevButton, SIGNAL(clicked(bool)), SLOT(trigger()));
      actionNext->connect(Controls->nextButton, SIGNAL(clicked(bool)), SLOT(trigger()));
      actionPlay->connect(Controls->playButton, SIGNAL(clicked(bool)), SLOT(trigger()));
      actionStop->connect(Controls->stopButton, SIGNAL(clicked(bool)), SLOT(trigger()));
      actionPause->connect(Controls->pauseButton, SIGNAL(clicked(bool)), SLOT(trigger()));
    }
  private:
    ControlOnToolbar<PlaybackControls> Controls;
    ControlOnToolbar<SeekControls> Seeking;
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
  qRegisterMetaType<ZXTune::Module::Information>("ZXTune::Module::Information");
  qRegisterMetaType<ZXTune::Module::Tracking>("ZXTune::Module::Tracking");
  qRegisterMetaType<ZXTune::Module::Analyze::ChannelsState>("ZXTune::Module::Analyze::ChannelsState");
  return QPointer<MainWindow>(new MainWindowImpl(argc, argv));
}
