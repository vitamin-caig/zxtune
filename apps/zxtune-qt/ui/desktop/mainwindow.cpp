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
#include "mainwindow.ui.h"
#include "ui/format.h"
#include "ui/utils.h"
#include "ui/controls/analyzer_control.h"
#include "ui/controls/playback_controls.h"
#include "ui/controls/seek_controls.h"
#include "ui/controls/status_control.h"
#include "ui/controls/volume_control.h"
#include "ui/informational/aboutdialog.h"
#include "ui/informational/componentsdialog.h"
#include "playlist/ui/container_view.h"
#include "supp/playback_supp.h"
#include <apps/base/app.h>
//common includes
#include <format.h>
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
    MainWindowImpl(Parameters::Container::Ptr options, const StringArray& cmdline)
      : Options(options)
      , Playback(PlaybackSupport::Create(*this, Options))
      , About(AboutDialog::Create(*this))
      , Components(ComponentsDialog::Create(*this))
      , Controls(PlaybackControls::Create(*this, *Playback))
      , Volume(VolumeControl::Create(*this, *Playback))
      , Status(StatusControl::Create(*this, *Playback))
      , Seeking(SeekControls::Create(*this, *Playback))
      , Analyzer(AnalyzerControl::Create(*this, *Playback))
      , MultiPlaylist(Playlist::UI::ContainerView::Create(*this, Options))
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
      MultiPlaylist->connect(Playback, SIGNAL(OnStartModule(ZXTune::Sound::Backend::Ptr)), SLOT(Play()));
      MultiPlaylist->connect(Playback, SIGNAL(OnResumeModule()), SLOT(Play()));
      MultiPlaylist->connect(Playback, SIGNAL(OnPauseModule()), SLOT(Pause()));
      MultiPlaylist->connect(Playback, SIGNAL(OnStopModule()), SLOT(Stop()));
      MultiPlaylist->connect(Playback, SIGNAL(OnFinishModule()), SLOT(Finish()));
      Playback->connect(MultiPlaylist, SIGNAL(OnItemActivated(const Playlist::Item::Data&)), SLOT(SetItem(const Playlist::Item::Data&)));
      this->connect(Playback, SIGNAL(OnStartModule(ZXTune::Sound::Backend::Ptr)), SLOT(StartModule(ZXTune::Sound::Backend::Ptr)));
      this->connect(Playback, SIGNAL(OnStopModule()), SLOT(StopModule()));

      StopModule();

      //TODO: remove
      {
        QStringList items;
        std::transform(cmdline.begin(), cmdline.end(),
          std::back_inserter(items), &ToQString);
        MultiPlaylist->CreatePlaylist(items);
      }
    }

    virtual void StartModule(ZXTune::Sound::Backend::Ptr player)
    {
      const ZXTune::Module::Information::Ptr info = player->GetModuleInformation();
      setWindowTitle(ToQString(Strings::Format(Text::TITLE_FORMAT,
        GetProgramTitle(),
        GetModuleTitle(Text::MODULE_TITLE_FORMAT, *info->Properties()))));
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
    const Parameters::Container::Ptr Options;
    PlaybackSupport* const Playback;
    AboutDialog* const About;
    ComponentsDialog* const Components;
    PlaybackControls* const Controls;
    VolumeControl* const Volume;
    StatusControl* const Status;
    SeekControls* const Seeking;
    AnalyzerControl* const Analyzer;
    Playlist::UI::ContainerView* const MultiPlaylist;
  };
}

QPointer<MainWindow> MainWindow::Create(Parameters::Container::Ptr options, const StringArray& cmdline)
{
  QPointer<MainWindow> res(new MainWindowImpl(options, cmdline));
  res->show();
  return res;
}
