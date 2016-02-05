/**
* 
* @file
*
* @brief Main window implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "mainwindow.h"
#include "mainwindow.ui.h"
#include "ui/state.h"
#include "ui/controls/analyzer_control.h"
#include "ui/controls/playback_controls.h"
#include "playlist/ui/container_view.h"
#include "supp/playback_supp.h"
//common includes
#include <contract.h>
#include <make_ptr.h>
//std includes
#include <fstream>
//boost includes
#include <boost/range/end.hpp>
//qt includes
#include <QtGui/QApplication>
#include <QtGui/QKeyEvent>
#include <QtGui/QLabel>
#include <QtGui/QStatusBar>
#include <QtGui/QToolBar>

namespace
{
  const char BACKLIGHT_CONTROL_DEVICE[] = "/proc/jz/lcd_backlight";

  class BacklightControl
  {
  public:
    BacklightControl()
      : PrevLevel(100)
      , Enabled(true)
    {
      if (!(std::ifstream(BACKLIGHT_CONTROL_DEVICE) >> PrevLevel))
      {
        PrevLevel = -1;
      }
    }
    
    void Enable()
    {
      if (!Enabled)
      {
        std::ofstream(BACKLIGHT_CONTROL_DEVICE) << PrevLevel << '\n';
        Enabled = true;
      }
    }
    
    void Disable()
    {
      if (Enabled)
      {
        std::ofstream(BACKLIGHT_CONTROL_DEVICE) << "0\n";
        Enabled = false;
      }
    }
  private:
    int PrevLevel;
    bool Enabled;
  };
  
  class EmbeddedMainWindowImpl : public EmbeddedMainWindow
                               , private Ui::MainWindowEmbedded
  {
  public:
    explicit EmbeddedMainWindowImpl(Parameters::Container::Ptr options)
      : Options(options)
      , Playback(PlaybackSupport::Create(*this, Options))
      , Controls(PlaybackControls::Create(*this, *Playback))
      , Analyzer(AnalyzerControl::Create(*this, *Playback))
      , Playlist(Playlist::UI::ContainerView::Create(*this, Options))
    {
      setupUi(this);
      State = UI::State::Create(*this);

      statusBar()->addWidget(new QLabel(
        tr("X-Space A-Ctrl B-Alt Y-Shift Select-Esc Start-Enter LH-Tab RH-Backspace"), this));
      //fill menu
      Controls->hide();
      menubar->addMenu(Controls->GetActionsMenu());
      menubar->addMenu(Playlist->GetActionsMenu());
      //fill toolbar and layout menu
      {
        QWidget* const ALL_WIDGETS[] =
        {
          AddWidgetOnLayout(Analyzer)
        };
        //playlist is mandatory and cannot be hidden
        AddWidgetOnLayout(Playlist);
        State->Load();
        std::for_each(ALL_WIDGETS, boost::end(ALL_WIDGETS), std::bind1st(std::mem_fun(&EmbeddedMainWindowImpl::AddWidgetLayoutControl), this));
      }

      //connect root actions
      Require(Playlist->connect(Controls, SIGNAL(OnPrevious()), SLOT(Prev())));
      Require(Playlist->connect(Controls, SIGNAL(OnNext()), SLOT(Next())));
      Require(Playlist->connect(Playback, SIGNAL(OnStartModule(Sound::Backend::Ptr, Playlist::Item::Data::Ptr)), SLOT(Play())));
      Require(Playlist->connect(Playback, SIGNAL(OnResumeModule()), SLOT(Play())));
      Require(Playlist->connect(Playback, SIGNAL(OnPauseModule()), SLOT(Pause())));
      Require(Playlist->connect(Playback, SIGNAL(OnStopModule()), SLOT(Stop())));
      Require(Playlist->connect(Playback, SIGNAL(OnFinishModule()), SLOT(Finish())));
      Require(Playback->connect(Playlist, SIGNAL(Activated(Playlist::Item::Data::Ptr)), SLOT(SetDefaultItem(Playlist::Item::Data::Ptr))));
      Require(Playback->connect(Playlist, SIGNAL(ItemActivated(Playlist::Item::Data::Ptr)), SLOT(SetItem(Playlist::Item::Data::Ptr))));
      Require(Playback->connect(Playlist, SIGNAL(Deactivated()), SLOT(ResetItem())));
      Require(connect(actionAddFiles, SIGNAL(triggered()), Playlist, SLOT(AddFiles())));
      Require(connect(actionAddFolder, SIGNAL(triggered()), Playlist, SLOT(AddFolder())));
      
      Playlist->Setup();
    }

    virtual void SetCmdline(const QStringList& /*args*/)
    {
    }

    //qwidget virtuals
    virtual void keyPressEvent(QKeyEvent* event)
    {
      if (event->key() == Qt::Key_Pause)
      {
        Backlight.Disable();
      }
      QWidget::keyPressEvent(event);
    }

    virtual void keyReleaseEvent(QKeyEvent* event)
    {
      if (event->key() == Qt::Key_Pause)
      {
        Backlight.Enable();
      }
      QWidget::keyPressEvent(event);
    }

    virtual void closeEvent(QCloseEvent* event)
    {
      Backlight.Enable();
      State->Save();
      Playlist->Teardown();
      event->accept();
    }
  private:
    void AddWidgetLayoutControl(QWidget* widget)
    {
      QAction* const action = new QAction(widget->windowTitle(), widget);
      action->setCheckable(true);
      action->setChecked(widget->isVisibleTo(this));
      //integrate
      menuLayout->addAction(action);
      Require(widget->connect(action, SIGNAL(toggled(bool)), SLOT(setVisible(bool))));
    }

    QWidget* AddWidgetOnLayout(QWidget* widget)
    {
      centralWidget()->layout()->addWidget(widget);
      State->AddWidget(*widget);
      return widget;
    }
  private:
    const Parameters::Container::Ptr Options;
    UI::State::Ptr State;
    BacklightControl Backlight;
    PlaybackSupport* const Playback;
    PlaybackControls* const Controls;
    AnalyzerControl* const Analyzer;
    Playlist::UI::ContainerView* const Playlist;
  };
}

MainWindow::Ptr EmbeddedMainWindow::Create(Parameters::Container::Ptr options)
{
  //TODO: create proper window
  const MainWindow::Ptr res = MakePtr<EmbeddedMainWindowImpl>(options);
  res->setWindowFlags(Qt::FramelessWindowHint);
  res->showMaximized();
  return res;
}
