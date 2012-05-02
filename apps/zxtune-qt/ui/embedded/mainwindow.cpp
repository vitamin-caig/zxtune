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
#include "mainwindow.h"
#include "mainwindow.ui.h"
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
  class EmbeddedMainWindowImpl : public EmbeddedMainWindow
                               , private Ui::MainWindowEmbedded
  {
  public:
    EmbeddedMainWindowImpl(Parameters::Container::Ptr options, const StringArray& /*cmdline*/)
      : Options(options)
      , Playback(PlaybackSupport::Create(*this, Options))
      , Controls(PlaybackControls::Create(*this, *Playback))
      , Analyzer(AnalyzerControl::Create(*this, *Playback))
      , Playlist(Playlist::UI::ContainerView::Create(*this, Options))
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
      Playlist->connect(Playback, SIGNAL(OnStartModule(ZXTune::Sound::Backend::Ptr, Playlist::Item::Data::Ptr)), SLOT(Play()));
      Playlist->connect(Playback, SIGNAL(OnResumeModule()), SLOT(Play()));
      Playlist->connect(Playback, SIGNAL(OnPauseModule()), SLOT(Pause()));
      Playlist->connect(Playback, SIGNAL(OnStopModule()), SLOT(Stop()));
      Playlist->connect(Playback, SIGNAL(OnFinishModule()), SLOT(Finish()));
      Require(Playback->connect(Playlist, SIGNAL(ItemActivated(Playlist::Item::Data::Ptr)), SLOT(SetItem(Playlist::Item::Data::Ptr))));
      this->connect(actionAddFiles, SIGNAL(triggered()), Playlist, SLOT(AddFiles()));
      this->connect(actionAddFolder, SIGNAL(triggered()), Playlist, SLOT(AddFolder()));
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
    const Parameters::Container::Ptr Options;
    PlaybackSupport* const Playback;
    PlaybackControls* const Controls;
    AnalyzerControl* const Analyzer;
    Playlist::UI::ContainerView* const Playlist;
  };
}

QPointer<EmbeddedMainWindow> EmbeddedMainWindow::Create(Parameters::Container::Ptr options, const StringArray& cmdline)
{
  //TODO: create proper window
  QPointer<EmbeddedMainWindow> res(new EmbeddedMainWindowImpl(options, cmdline));
  res->setWindowFlags(Qt::FramelessWindowHint);
  res->showMaximized();
  return res;
}
