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
#include "ui/state.h"
#include "ui/controls/analyzer_control.h"
#include "ui/controls/playback_controls.h"
#include "ui/controls/playback_options.h"
#include "ui/controls/seek_controls.h"
#include "ui/controls/status_control.h"
#include "ui/controls/volume_control.h"
#include "ui/informational/aboutdialog.h"
#include "ui/informational/componentsdialog.h"
#include "ui/preferences/preferencesdialog.h"
#include "ui/tools/errordialog.h"
#include "playlist/ui/container_view.h"
#include "supp/playback_supp.h"
#include <apps/version/api.h>
//common includes
#include <contract.h>
#include <format.h>
//library includes
#include <core/module_attrs.h>
//boost includes
#include <boost/bind.hpp>
//qt includes
#include <QtCore/QUrl>
#include <QtGui/QApplication>
#include <QtGui/QCloseEvent>
#include <QtGui/QDesktopServices>
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
      , Controls(PlaybackControls::Create(*this, *Playback))
      , FastOptions(PlaybackOptions::Create(*this, *Playback, Options))
      , Volume(VolumeControl::Create(*this, *Playback))
      , Status(StatusControl::Create(*this, *Playback))
      , Seeking(SeekControls::Create(*this, *Playback))
      , Analyzer(AnalyzerControl::Create(*this, *Playback))
      , MultiPlaylist(Playlist::UI::ContainerView::Create(*this, Options))
      , Playing(false)
    {
      setupUi(this);
      State = UI::State::Create(*this);
      //fill menu
      menubar->addMenu(Controls->GetActionsMenu());
      menubar->addMenu(MultiPlaylist->GetActionsMenu());
      menubar->addMenu(menuHelp);
      //fill toolbar and layout menu
      {
        QWidget* const ALL_WIDGETS[] =
        {
          AddWidgetOnToolbar(Controls, false),
          AddWidgetOnToolbar(FastOptions, false),
          AddWidgetOnToolbar(Volume, true),
          AddWidgetOnToolbar(Status, false),
          AddWidgetOnToolbar(Seeking, true),
          AddWidgetOnToolbar(Analyzer, false)
        };
        //playlist is mandatory and cannot be hidden
        AddWidgetOnLayout(MultiPlaylist);
        State->Load();
        std::for_each(ALL_WIDGETS, ArrayEnd(ALL_WIDGETS), boost::bind(&MainWindowImpl::AddWidgetLayoutControl, this, _1));
      }

      //connect root actions
      Require(connect(actionComponents, SIGNAL(triggered()), SLOT(ShowComponentsInformation())));
      Require(connect(actionAbout, SIGNAL(triggered()), SLOT(ShowAboutProgram())));
      this->connect(actionOnlineHelp, SIGNAL(triggered()), SLOT(VisitHelp()));
      this->connect(actionWebSite, SIGNAL(triggered()), SLOT(VisitSite()));
      this->connect(actionOnlineFAQ, SIGNAL(triggered()), SLOT(VisitFAQ()));
      this->connect(actionReportBug, SIGNAL(triggered()), SLOT(ReportIssue()));
      this->connect(actionAboutQt, SIGNAL(triggered()), SLOT(ShowAboutQt()));
      this->connect(actionPreferences, SIGNAL(triggered()), SLOT(ShowPreferences()));

      MultiPlaylist->connect(Controls, SIGNAL(OnPrevious()), SLOT(Prev()));
      MultiPlaylist->connect(Controls, SIGNAL(OnNext()), SLOT(Next()));
      MultiPlaylist->connect(Playback, SIGNAL(OnStartModule(ZXTune::Sound::Backend::Ptr, Playlist::Item::Data::Ptr)), SLOT(Play()));
      MultiPlaylist->connect(Playback, SIGNAL(OnResumeModule()), SLOT(Play()));
      MultiPlaylist->connect(Playback, SIGNAL(OnPauseModule()), SLOT(Pause()));
      MultiPlaylist->connect(Playback, SIGNAL(OnStopModule()), SLOT(Stop()));
      MultiPlaylist->connect(Playback, SIGNAL(OnFinishModule()), SLOT(Finish()));
      Require(Playback->connect(MultiPlaylist, SIGNAL(ItemActivated(Playlist::Item::Data::Ptr)), SLOT(SetItem(Playlist::Item::Data::Ptr))));
      this->connect(Playback, SIGNAL(OnStartModule(ZXTune::Sound::Backend::Ptr, Playlist::Item::Data::Ptr)),
        SLOT(StartModule(ZXTune::Sound::Backend::Ptr, Playlist::Item::Data::Ptr)));
      this->connect(Playback, SIGNAL(OnStopModule()), SLOT(StopModule()));
      Require(connect(Playback, SIGNAL(ErrorOccurred(const Error&)), SLOT(ShowError(const Error&))));
      this->connect(actionAddFiles, SIGNAL(triggered()), MultiPlaylist, SLOT(AddFiles()));
      this->connect(actionAddFolder, SIGNAL(triggered()), MultiPlaylist, SLOT(AddFolder()));

      StopModule();

      //TODO: remove
      {
        QStringList items;
        std::transform(cmdline.begin(), cmdline.end(),
          std::back_inserter(items), &ToQString);
        MultiPlaylist->Setup(items);
      }
    }

    virtual void StartModule(ZXTune::Sound::Backend::Ptr /*player*/, Playlist::Item::Data::Ptr item)
    {
      setWindowTitle(ToQString(Strings::Format(Text::TITLE_FORMAT,
        GetProgramTitle(),
        item->GetDisplayName())));
      Playing = true;
    }

    virtual void StopModule()
    {
      Playing = false;
      setWindowTitle(ToQString(GetProgramTitle()));
    }
    
    virtual void ShowPreferences()
    {
      UI::ShowPreferencesDialog(*this, Playing);
    }

    virtual void ShowComponentsInformation()
    {
      UI::ShowComponentsInformation(*this);
    }

    virtual void ShowAboutProgram()
    {
      UI::ShowProgramInformation(*this);
    }

    virtual void ShowAboutQt()
    {
      QMessageBox::aboutQt(this);
    }

    virtual void VisitHelp()
    {
      const QString siteUrl(Text::HELP_URL);
      QDesktopServices::openUrl(QUrl(siteUrl));
    }

    virtual void VisitSite()
    {
      const QString siteUrl(Text::HOMEPAGE_URL);
      QDesktopServices::openUrl(QUrl(siteUrl));
    }

    virtual void VisitFAQ()
    {
      const QString faqUrl(Text::FAQ_URL);
      QDesktopServices::openUrl(QUrl(faqUrl));
    }

    virtual void ReportIssue()
    {
      const QString faqUrl(Text::REPORT_BUG_URL);
      QDesktopServices::openUrl(QUrl(faqUrl));
    }

    virtual void ShowError(const Error& err)
    {
      ShowErrorMessage(QString(), err);
    }

    //QWidgets virtuals
    virtual void closeEvent(QCloseEvent* event)
    {
      Playback->Stop();
      State->Save();
      MultiPlaylist->Teardown();
      event->accept();
    }
  private:
    QToolBar* AddWidgetOnToolbar(QWidget* widget, bool lastInRow)
    {
      const QString widgetId = widget->windowTitle();
      QToolBar* const toolBar = new QToolBar(widgetId, this);
      toolBar->setObjectName(widgetId);
      QSizePolicy sizePolicy(QSizePolicy::Maximum, QSizePolicy::MinimumExpanding);
      sizePolicy.setHorizontalStretch(0);
      sizePolicy.setVerticalStretch(0);
      sizePolicy.setHeightForWidth(toolBar->sizePolicy().hasHeightForWidth());
      toolBar->setSizePolicy(sizePolicy);
      toolBar->setAllowedAreas(Qt::TopToolBarArea);
      toolBar->setFloatable(false);
      toolBar->setContextMenuPolicy(Qt::PreventContextMenu);
      toolBar->addWidget(widget);

      addToolBar(Qt::TopToolBarArea, toolBar);
      if (lastInRow)
      {
        addToolBarBreak();
      }
      return toolBar;
    }

    void AddWidgetLayoutControl(QWidget* widget)
    {
      QAction* const action = new QAction(widget->windowTitle(), widget);
      action->setCheckable(true);
      action->setChecked(widget->isVisibleTo(this));
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
    UI::State::Ptr State;
    PlaybackSupport* const Playback;
    PlaybackControls* const Controls;
    PlaybackOptions* const FastOptions;
    VolumeControl* const Volume;
    StatusControl* const Status;
    SeekControls* const Seeking;
    AnalyzerControl* const Analyzer;
    Playlist::UI::ContainerView* const MultiPlaylist;
    bool Playing;
  };
}

QPointer<MainWindow> MainWindow::Create(Parameters::Container::Ptr options, const StringArray& cmdline)
{
  QPointer<MainWindow> res(new MainWindowImpl(options, cmdline));
  res->show();
  return res;
}
