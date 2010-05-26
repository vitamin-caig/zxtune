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
#include "ui/controls/analyzer_control.h"
#include "ui/controls/playback_controls.h"
#include "ui/controls/seek_controls.h"
#include "ui/controls/status_control.h"
#include "ui/playlist/playlist.h"
#include "supp/playback_supp.h"
//common includes
#include <logging.h>
//qt includes
#include <QtGui/QApplication>
#include <QtGui/QToolBar>

namespace
{
  class LayoutControl
  {
  public:
    LayoutControl(QMainWindow* mainWindow, QWidget* controlled,
                     QMenu* layoutMenu, const char* menuTitle)
      : Action(new QAction(mainWindow))
    {
      //setup action
      Action->setCheckable(true);
      Action->setChecked(true);//TODO
      Action->setText(QApplication::translate(mainWindow->objectName().toStdString().c_str(), menuTitle, 0, QApplication::UnicodeUTF8));
      //integrate
      layoutMenu->addAction(Action);
      controlled->connect(Action, SIGNAL(toggled(bool)), SLOT(setVisible(bool)));
    }
  private:
    QAction* const Action;
  };

  template<class T>
  class ToolbarControl
  {
  public:
    ToolbarControl(QMainWindow* mainWindow, QMenu* layoutMenu, const char* menuTitle)
      : Control(T::Create(mainWindow))
      , Toolbar(new QToolBar(mainWindow))
      , Layout(mainWindow, Toolbar, layoutMenu, menuTitle)
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
    const LayoutControl Layout;
  };

  template<class T>
  class WidgetControl
  {
  public:
    WidgetControl(QMainWindow* mainWindow, QMenu* layoutMenu, const char* menuTitle)
      : Control(T::Create(mainWindow))
      , Layout(mainWindow, Control, layoutMenu, menuTitle)
    {
      mainWindow->centralWidget()->layout()->addWidget(Control);
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
    const LayoutControl Layout;
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
      , Collection(this, menuLayout, "Playlist")
      , Analyzer(this, menuLayout, "Analyzer")
      , Status(this, menuLayout, "Status")
      , Playback(PlaybackSupport::Create(this))
    {
      //TODO: remove
      for (int param = 1; param < argc; ++param)
      {
        Collection->AddItemByPath(FromStdString(argv[param]));
      }

      //connect root actions
      Collection->connect(Controls, SIGNAL(OnPrevious()), SLOT(PrevItem()));
      Collection->connect(Controls, SIGNAL(OnNext()), SLOT(NextItem()));
      Collection->connect(Playback, SIGNAL(OnStartModule(const ZXTune::Module::Information&)), SLOT(PlayItem()));
      Collection->connect(Playback, SIGNAL(OnResumeModule(const ZXTune::Module::Information&)), SLOT(PlayItem()));
      Collection->connect(Playback, SIGNAL(OnPauseModule(const ZXTune::Module::Information&)), SLOT(PauseItem()));
      Collection->connect(Playback, SIGNAL(OnStopModule(const ZXTune::Module::Information&)), SLOT(StopItem()));
      Collection->connect(Playback, SIGNAL(OnFinishModule(const ZXTune::Module::Information&)), SLOT(NextItem()));
      Playback->connect(Collection, SIGNAL(OnItemSet(const Playitem&)), SLOT(SetItem(const Playitem&)));
      Playback->connect(Collection, SIGNAL(OnItemSelected(const Playitem&)), SLOT(SelectItem(const Playitem&)));
      Playback->connect(Controls, SIGNAL(OnPlay()), SLOT(Play()));
      Playback->connect(Controls, SIGNAL(OnStop()), SLOT(Stop()));
      Playback->connect(Controls, SIGNAL(OnPause()), SLOT(Pause()));
      Playback->connect(Seeking, SIGNAL(OnSeeking(int)), SLOT(Seek(int)));
      Seeking->connect(Playback, SIGNAL(OnStartModule(const ZXTune::Module::Information&)), SLOT(InitState(const ZXTune::Module::Information&)));
      Seeking->connect(Playback, SIGNAL(OnUpdateState(uint, const ZXTune::Module::Tracking&, const ZXTune::Module::Analyze::ChannelsState&)),
        SLOT(UpdateState(uint)));
      Seeking->connect(Playback, SIGNAL(OnStopModule(const ZXTune::Module::Information&)), SLOT(CloseState()));
      Analyzer->connect(Playback, SIGNAL(OnStopModule(const ZXTune::Module::Information&)), SLOT(InitState()));
      Analyzer->connect(Playback, SIGNAL(OnUpdateState(uint, const ZXTune::Module::Tracking&, const ZXTune::Module::Analyze::ChannelsState&)),
        SLOT(UpdateState(uint, const ZXTune::Module::Tracking&, const ZXTune::Module::Analyze::ChannelsState&)));
      Status->connect(Playback, SIGNAL(OnStartModule(const ZXTune::Module::Information&)), SLOT(InitState(const ZXTune::Module::Information&)));
      Status->connect(Playback, SIGNAL(OnUpdateState(uint, const ZXTune::Module::Tracking&, const ZXTune::Module::Analyze::ChannelsState&)),
        SLOT(UpdateState(uint, const ZXTune::Module::Tracking&, const ZXTune::Module::Analyze::ChannelsState&)));
      Status->connect(Playback, SIGNAL(OnStopModule(const ZXTune::Module::Information&)), SLOT(CloseState()));
    }
  private:
    ToolbarControl<PlaybackControls> Controls;
    ToolbarControl<SeekControls> Seeking;
    WidgetControl<Playlist> Collection;
    ToolbarControl<AnalyzerControl> Analyzer;
    ToolbarControl<StatusControl> Status;
    PlaybackSupport* const Playback;
  };
}

QPointer<MainWindow> MainWindow::Create(int argc, char* argv[])
{
  QPointer<MainWindow> res(new MainWindowImpl(argc, argv));
  res->show();
  return res;
}

QPointer<QMainWindow> CreateMainWindow(int argc, char* argv[])
{
  return QPointer<QMainWindow>(MainWindow::Create(argc, argv));
}
