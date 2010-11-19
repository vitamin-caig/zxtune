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
#include "mainwindow_embedded.ui.h"
#include "ui/controls/analyzer_control.h"
#include "ui/controls/playback_controls.h"
#include "playlist/ui/container_view.h"
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
  class MainWindowEmbeddedImpl : public MainWindowEmbedded
                               , private Ui::MainWindowEmbedded
  {
  public:
    MainWindowEmbeddedImpl(int /*argc*/, char* /*argv*/[])
      //, Seeking(this, menuLayout, "Seeking")
      : Controls(PlaybackControls::Create(*this))
      , Analyzer(AnalyzerControl::Create(*this))
      , Playlist(Playlist::UI::ContainerView::Create(*this))
      , Playback(PlaybackSupport::Create(*this))
    {
      setupUi(this);
      statusBar()->addWidget(new QLabel(
        tr("X-Space A-Ctrl B-Alt Y-Shift Select-Esc Start-Enter LH-Tab RH-Backspace"), this));
      //fill menu
      menubar->addMenu(Controls->GetActionsMenu());
      menubar->addMenu(Playlist->GetActionsMenu());
      //fill toolbar and layout menu
      AddWidgetWithLayoutControl(AddWidgetOnLayout(Analyzer));
      AddWidgetWithLayoutControl(AddWidgetOnLayout(Playlist));

      //connect root actions
      Playlist->connect(Controls, SIGNAL(OnPrevious()), SLOT(Prev()));
      Playlist->connect(Controls, SIGNAL(OnNext()), SLOT(Next()));
      Playlist->connect(Playback, SIGNAL(OnStartModule(ZXTune::Module::Player::ConstPtr)), SLOT(Play()));
      Playlist->connect(Playback, SIGNAL(OnResumeModule()), SLOT(Play()));
      Playlist->connect(Playback, SIGNAL(OnPauseModule()), SLOT(Pause()));
      Playlist->connect(Playback, SIGNAL(OnStopModule()), SLOT(Stop()));
      Playlist->connect(Playback, SIGNAL(OnFinishModule()), SLOT(Finish()));
      Playback->connect(Playlist, SIGNAL(OnItemActivated(const Playitem&)), SLOT(SetItem(const Playitem&)));
      Playback->connect(Controls, SIGNAL(OnPlay()), SLOT(Play()));
      Playback->connect(Controls, SIGNAL(OnStop()), SLOT(Stop()));
      Playback->connect(Controls, SIGNAL(OnPause()), SLOT(Pause()));
      Analyzer->connect(Playback, SIGNAL(OnStartModule(ZXTune::Module::Player::ConstPtr)), SLOT(InitState(ZXTune::Module::Player::ConstPtr)));
      Analyzer->connect(Playback, SIGNAL(OnStopModule()), SLOT(CloseState()));
      Analyzer->connect(Playback, SIGNAL(OnUpdateState()), SLOT(UpdateState()));
    }
  private:
    void AddWidgetWithLayoutControl(QWidget* widget)
    {
      QAction* const action = new QAction(widget);
      action->setCheckable(true);
      action->setChecked(true);//TODO
      action->setText(widget->windowTitle());
      //integrate
      menuLayout->addAction(action);
      widget->connect(action, SIGNAL(toggled(bool)), SLOT(setVisible(bool)));
    }

    QWidget* AddWidgetOnLayout(QWidget* widget)
    {
      centralWidget()->layout()->addWidget(widget);
      return widget;
    }
  private:
    PlaybackControls* const Controls;
    AnalyzerControl* const Analyzer;
    Playlist::UI::ContainerView* const Playlist;
    PlaybackSupport* const Playback;
  };
}

QPointer<MainWindowEmbedded> MainWindowEmbedded::Create(int argc, char* argv[])
{
  //TODO: create proper window
  QPointer<MainWindowEmbedded> res(new MainWindowEmbeddedImpl(argc, argv));
  res->setWindowFlags(Qt::FramelessWindowHint);
  res->showMaximized();
  return res;
}

QPointer<QMainWindow> CreateMainWindow(int argc, char* argv[])
{
  return QPointer<QMainWindow>(MainWindowEmbedded::Create(argc, argv));
}
