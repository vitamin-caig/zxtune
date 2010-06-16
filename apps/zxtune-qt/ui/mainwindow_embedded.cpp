/*
Abstract:
  Main window embedded implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "mainwindow_embedded.h"
#include "mainwindow_embedded_ui.h"
#include "mainwindow_embedded_moc.h"
#include "ui/controls/analyzer_control.h"
#include "ui/controls/playback_controls.h"
#include "ui/playlist/playlist.h"
#include "supp/playback_supp.h"
//common includes
#include <logging.h>
//qt includes
#include <QtGui/QApplication>
#include <QtGui/QLabel>
#include <QtGui/QStatusBar>
#include <QtGui/QToolBar>

namespace
{
  template<class T>
  class WidgetControl
  {
  public:
    WidgetControl(QMainWindow* mainWindow, bool visible)
      : Control(T::Create(mainWindow))
    {
      mainWindow->centralWidget()->layout()->addWidget(Control);
      Control->setVisible(visible);
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
  };

  
  class UiHelper : public Ui::MainWindow
  {
  public:
    explicit UiHelper(QMainWindow* mainWindow)
    {
      setupUi(mainWindow);
    }
  };

  class MainWindowEmbeddedImpl : public MainWindowEmbedded
                               , private UiHelper
  {
  public:
    MainWindowEmbeddedImpl(int /*argc*/, char* /*argv*/[])
      : UiHelper(this)
      //, Seeking(this, menuLayout, "Seeking")
      , Controls(this, false)
      , Analyzer(this, true)
      , Collection(this, true)
      , Playback(PlaybackSupport::Create(this))
    {
      statusBar()->addWidget(new QLabel(
        QString::fromUtf8("X-Space A-Ctrl B-Alt Y-Shift Select-Esc Start-Enter LH-Tab RH-Backspace"), this));

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
      //Playback->connect(Seeking, SIGNAL(OnSeeking(int)), SLOT(Seek(int)));
      //Seeking->connect(Playback, SIGNAL(OnStartModule(const ZXTune::Module::Information&)), SLOT(InitState(const ZXTune::Module::Information&)));
      //Seeking->connect(Playback, SIGNAL(OnUpdateState(uint, const ZXTune::Module::Tracking&, const ZXTune::Module::Analyze::ChannelsState&)),
        //SLOT(UpdateState(uint)));
      //Seeking->connect(Playback, SIGNAL(OnStopModule(const ZXTune::Module::Information&)), SLOT(CloseState(const ZXTune::Module::Information&)));
      Analyzer->connect(Playback, SIGNAL(OnStopModule(const ZXTune::Module::Information&)), SLOT(InitState()));
      Analyzer->connect(Playback, SIGNAL(OnUpdateState(const ZXTune::Module::State&, const ZXTune::Module::Analyze::ChannelsState&)),
        SLOT(UpdateState(const ZXTune::Module::State&, const ZXTune::Module::Analyze::ChannelsState&)));
    }
  private:
    WidgetControl<PlaybackControls> Controls;
    //ToolbarControl<SeekControls> Seeking;
    WidgetControl<AnalyzerControl> Analyzer;
    WidgetControl<Playlist> Collection;
    PlaybackSupport* const Playback;
  };
}

QPointer<MainWindowEmbedded> MainWindowEmbedded::Create(int argc, char* argv[])
{
  qApp->setFont(QFont(QString::fromUtf8("DejaVuSans")));
  //TODO: create proper window
  QPointer<MainWindowEmbedded> res(new MainWindowEmbeddedImpl(argc, argv));
  QApplication::setOverrideCursor(QCursor(Qt::BlankCursor));
  res->setWindowFlags(Qt::FramelessWindowHint);
  res->showMaximized();
  return res;
}

QPointer<QMainWindow> CreateMainWindow(int argc, char* argv[])
{
  return QPointer<QMainWindow>(MainWindowEmbedded::Create(argc, argv));
}
