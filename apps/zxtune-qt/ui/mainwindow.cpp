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
    ToolbarControl(QMainWindow* mainWindow, QMenu* layoutMenu, const char* menuTitle, bool lastInRow)
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
      Toolbar->setContextMenuPolicy(Qt::PreventContextMenu);
      mainWindow->addToolBar(Qt::TopToolBarArea, Toolbar);
      if (lastInRow)
      {
        mainWindow->addToolBarBreak();
      }
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
      , About(AboutDialog::Create(this))
      , Components(ComponentsDialog::Create(this))
      , Controls(this, menuLayout, "Controls", false)
      , Volume(this, menuLayout, "Volume", true)
      , Status(this, menuLayout, "Status", false)
      , Seeking(this, menuLayout, "Seeking", true)
      , Analyzer(this, menuLayout, "Analyzer", true)
      , Collection(this, menuLayout, "Playlist")
      , Playback(PlaybackSupport::Create(this))
    {
      //TODO: remove
      {
        QStringList items;
        for (int param = 1; param < argc; ++param)
        {
          items.append(QString::fromUtf8(argv[param]));
        }
        Collection->CreatePlaylist(items);
      }
      //fill menu
      menubar->addMenu(Controls->GetActionsMenu());
      menubar->addMenu(Collection->GetActionsMenu());

      Components->connect(actionComponents, SIGNAL(triggered()), SLOT(Show()));
      About->connect(actionAbout, SIGNAL(triggered()), SLOT(Show()));
      this->connect(actionAboutQt, SIGNAL(triggered()), SLOT(ShowAboutQt()));
      //connect root actions
      Collection->connect(Controls, SIGNAL(OnPrevious()), SLOT(Prev()));
      Collection->connect(Controls, SIGNAL(OnNext()), SLOT(Next()));
      Collection->connect(Playback, SIGNAL(OnStartModule(ZXTune::Module::Player::ConstPtr)), SLOT(Play()));
      Collection->connect(Playback, SIGNAL(OnResumeModule()), SLOT(Play()));
      Collection->connect(Playback, SIGNAL(OnPauseModule()), SLOT(Pause()));
      Collection->connect(Playback, SIGNAL(OnStopModule()), SLOT(Stop()));
      Collection->connect(Playback, SIGNAL(OnFinishModule()), SLOT(Finish()));
      Playback->connect(Collection, SIGNAL(OnItemActivated(const Playitem&)), SLOT(SetItem(const Playitem&)));
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
    AboutDialog* const About;
    ComponentsDialog* const Components;
    ToolbarControl<PlaybackControls> Controls;
    ToolbarControl<VolumeControl> Volume;
    ToolbarControl<StatusControl> Status;
    ToolbarControl<SeekControls> Seeking;
    ToolbarControl<AnalyzerControl> Analyzer;
    WidgetControl<PlaylistContainerView> Collection;
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
