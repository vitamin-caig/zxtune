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
#include "utils.h"
#include "format.h"
#include "mainwindow.h"
#include "mainwindow_ui.h"
#include "mainwindow_moc.h"
#include "ui/controls/analyzer_control.h"
#include "ui/controls/playback_controls.h"
#include "ui/controls/seek_controls.h"
#include "ui/controls/status_control.h"
#include "ui/controls/volume_control.h"
#include "ui/informational/aboutdialog.h"
#include "ui/informational/componentsdialog.h"
#include "ui/playlist/playlist_container.h"
#include "supp/playback_supp.h"
#include <apps/base/app.h>
//common includes
#include <formatter.h>
#include <logging.h>
//library includes
#include <core/module_attrs.h>
//qt includes
#include <QtGui/QApplication>
#include <QtGui/QMessageBox>
#include <QtGui/QToolBar>
//text includes
#include "text/text.h"

namespace
{
  class MainWindowImpl : public MainWindow
                       , public Ui::MainWindow
  {
  public:
    MainWindowImpl(int argc, char* argv[])
      : About(AboutDialog::Create(this))
      , Components(ComponentsDialog::Create(this))
      , Controls(PlaybackControls::Create(this))
      , Volume(VolumeControl::Create(this))
      , Status(StatusControl::Create(this))
      , Seeking(SeekControls::Create(this))
      , Analyzer(AnalyzerControl::Create(this))
      , MultiPlaylist(PlaylistContainerView::Create(this))
      , Playback(PlaybackSupport::Create(this))
    {
      setupUi(this);
      //fill menu
      menubar->addMenu(Controls->GetActionsMenu());
      menubar->addMenu(MultiPlaylist->GetActionsMenu());
      //fill toolbar and layout menu
      AddWidgetWithLayoutControl(AddWidgetOnToolbar(Controls, false));
      AddWidgetWithLayoutControl(AddWidgetOnToolbar(Volume, true));
      AddWidgetWithLayoutControl(AddWidgetOnToolbar(Status, false));
      AddWidgetWithLayoutControl(AddWidgetOnToolbar(Seeking, true));
      AddWidgetWithLayoutControl(AddWidgetOnToolbar(Analyzer, false));
      AddWidgetWithLayoutControl(AddWidgetOnLayout(MultiPlaylist));

      //connect root actions
      Components->connect(actionComponents, SIGNAL(triggered()), SLOT(Show()));
      About->connect(actionAbout, SIGNAL(triggered()), SLOT(Show()));
      this->connect(actionAboutQt, SIGNAL(triggered()), SLOT(ShowAboutQt()));

      MultiPlaylist->connect(Controls, SIGNAL(OnPrevious()), SLOT(Prev()));
      MultiPlaylist->connect(Controls, SIGNAL(OnNext()), SLOT(Next()));
      MultiPlaylist->connect(Playback, SIGNAL(OnStartModule(ZXTune::Module::Player::ConstPtr)), SLOT(Play()));
      MultiPlaylist->connect(Playback, SIGNAL(OnResumeModule()), SLOT(Play()));
      MultiPlaylist->connect(Playback, SIGNAL(OnPauseModule()), SLOT(Pause()));
      MultiPlaylist->connect(Playback, SIGNAL(OnStopModule()), SLOT(Stop()));
      MultiPlaylist->connect(Playback, SIGNAL(OnFinishModule()), SLOT(Finish()));
      Playback->connect(MultiPlaylist, SIGNAL(OnItemActivated(const Playitem&)), SLOT(SetItem(const Playitem&)));
      Playback->connect(Controls, SIGNAL(OnPlay()), SLOT(Play()));
      Playback->connect(Controls, SIGNAL(OnStop()), SLOT(Stop()));
      Playback->connect(Controls, SIGNAL(OnPause()), SLOT(Pause()));
      Playback->connect(Seeking, SIGNAL(OnSeeking(int)), SLOT(Seek(int)));
      Seeking->connect(Playback, SIGNAL(OnStartModule(ZXTune::Module::Player::ConstPtr)), SLOT(InitState(ZXTune::Module::Player::ConstPtr)));
      Seeking->connect(Playback, SIGNAL(OnUpdateState()), SLOT(UpdateState()));
      Seeking->connect(Playback, SIGNAL(OnStopModule()), SLOT(CloseState()));
      Analyzer->connect(Playback, SIGNAL(OnStartModule(ZXTune::Module::Player::ConstPtr)), SLOT(InitState(ZXTune::Module::Player::ConstPtr)));
      Analyzer->connect(Playback, SIGNAL(OnStopModule()), SLOT(CloseState()));
      Analyzer->connect(Playback, SIGNAL(OnUpdateState()), SLOT(UpdateState()));
      Status->connect(Playback, SIGNAL(OnStartModule(ZXTune::Module::Player::ConstPtr)), SLOT(InitState(ZXTune::Module::Player::ConstPtr)));
      Status->connect(Playback, SIGNAL(OnUpdateState()), SLOT(UpdateState()));
      Status->connect(Playback, SIGNAL(OnStopModule()), SLOT(CloseState()));
      Volume->connect(Playback, SIGNAL(OnUpdateState()), SLOT(UpdateState()));
      Volume->connect(Playback, SIGNAL(OnSetBackend(const ZXTune::Sound::Backend&)), SLOT(SetBackend(const ZXTune::Sound::Backend&)));
      this->connect(Playback, SIGNAL(OnStartModule(ZXTune::Module::Player::ConstPtr)), SLOT(StartModule(ZXTune::Module::Player::ConstPtr)));
      this->connect(Playback, SIGNAL(OnStopModule()), SLOT(StopModule()));

      StopModule();

      //TODO: remove
      {
        QStringList items;
        for (int param = 1; param < argc; ++param)
        {
          items.append(QString::fromUtf8(argv[param]));
        }
        MultiPlaylist->CreatePlaylist(items);
      }
    }

    virtual void StartModule(ZXTune::Module::Player::ConstPtr player)
    {
      const ZXTune::Module::Information::Ptr info = player->GetInformation();
      setWindowTitle(ToQString((Formatter(Text::TITLE_FORMAT)
        % GetProgramTitle()
        % GetModuleTitle(Text::MODULE_TITLE_FORMAT, *info->Properties())).str()));
    }

    virtual void StopModule()
    {
      setWindowTitle(ToQString(GetProgramTitle()));
    }

    virtual void ShowAboutQt()
    {
      QMessageBox::aboutQt(this);
    }
  private:
    QToolBar* AddWidgetOnToolbar(QWidget* widget, bool lastInRow)
    {
      QToolBar* const toolBar = new QToolBar(this);
      QSizePolicy sizePolicy(QSizePolicy::Maximum, QSizePolicy::MinimumExpanding);
      sizePolicy.setHorizontalStretch(0);
      sizePolicy.setVerticalStretch(0);
      sizePolicy.setHeightForWidth(toolBar->sizePolicy().hasHeightForWidth());
      toolBar->setSizePolicy(sizePolicy);
      toolBar->setAllowedAreas(Qt::TopToolBarArea);
      toolBar->setFloatable(false);
      toolBar->setContextMenuPolicy(Qt::PreventContextMenu);
      toolBar->addWidget(widget);
      toolBar->setWindowTitle(widget->windowTitle());

      addToolBar(Qt::TopToolBarArea, toolBar);
      if (lastInRow)
      {
        addToolBarBreak();
      }
      return toolBar;
    }

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
    AboutDialog* const About;
    ComponentsDialog* const Components;
    PlaybackControls* const Controls;
    VolumeControl* const Volume;
    StatusControl* const Status;
    SeekControls* const Seeking;
    AnalyzerControl* const Analyzer;
    PlaylistContainerView* const MultiPlaylist;
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
