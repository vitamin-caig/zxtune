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
#include "language.h"
#include "ui/format.h"
#include "ui/utils.h"
#include "ui/state.h"
#include "ui/parameters.h"
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
#include "update/check.h"
#include <apps/version/api.h>
//common includes
#include <contract.h>
//library includes
#include <core/module_attrs.h>
#include <debug/log.h>
#include <strings/format.h>
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
  const Debug::Stream Dbg("UI::MainWindow");

  UI::Language::Ptr CreateLanguage(const Parameters::Container& options)
  {
    const UI::Language::Ptr res = UI::Language::Create();
    Parameters::StringType lang = FromQString(res->GetSystem());
    options.FindValue(Parameters::ZXTuneQT::UI::LANGUAGE, lang);
    res->Set(ToQString(lang));
    return res;
  }

  class MainWindowImpl : public MainWindow
                       , public Ui::MainWindow
  {
  public:
    MainWindowImpl(Parameters::Container::Ptr options, const Strings::Array& cmdline)
      : Options(options)
      , Language(CreateLanguage(*options))
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
        Toolbars.push_back(AddWidgetOnToolbar(Controls, false));
        Toolbars.push_back(AddWidgetOnToolbar(FastOptions, false));
        Toolbars.push_back(AddWidgetOnToolbar(Volume, true));
        Toolbars.push_back(AddWidgetOnToolbar(Status, false));
        Toolbars.push_back(AddWidgetOnToolbar(Seeking, true));
        Toolbars.push_back(AddWidgetOnToolbar(Analyzer, false));
        //playlist is mandatory and cannot be hidden
        AddWidgetOnLayout(MultiPlaylist);
        State->Load();
        FillLayoutMenu();
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
      if (Update::CheckOperation* op = Update::CheckOperation::Create(*this))
      {
        Require(op->connect(actionCheckUpdates, SIGNAL(triggered()), SLOT(Execute())));
        Require(this->connect(op, SIGNAL(ErrorOccurred(const Error&)), SLOT(ShowError(const Error&))));
      }
      else
      {
        actionCheckUpdates->setEnabled(false);
      }

      MultiPlaylist->connect(Controls, SIGNAL(OnPrevious()), SLOT(Prev()));
      MultiPlaylist->connect(Controls, SIGNAL(OnNext()), SLOT(Next()));
      MultiPlaylist->connect(Playback, SIGNAL(OnStartModule(Sound::Backend::Ptr, Playlist::Item::Data::Ptr)), SLOT(Play()));
      MultiPlaylist->connect(Playback, SIGNAL(OnResumeModule()), SLOT(Play()));
      MultiPlaylist->connect(Playback, SIGNAL(OnPauseModule()), SLOT(Pause()));
      MultiPlaylist->connect(Playback, SIGNAL(OnStopModule()), SLOT(Stop()));
      MultiPlaylist->connect(Playback, SIGNAL(OnFinishModule()), SLOT(Finish()));
      Require(Playback->connect(MultiPlaylist, SIGNAL(ItemActivated(Playlist::Item::Data::Ptr)), SLOT(SetItem(Playlist::Item::Data::Ptr))));
      Require(Playback->connect(MultiPlaylist, SIGNAL(Deactivated()), SLOT(ResetItem())));
      this->connect(Playback, SIGNAL(OnStartModule(Sound::Backend::Ptr, Playlist::Item::Data::Ptr)),
        SLOT(StartModule(Sound::Backend::Ptr, Playlist::Item::Data::Ptr)));
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

    virtual void StartModule(Sound::Backend::Ptr /*player*/, Playlist::Item::Data::Ptr item)
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
      const QLatin1String siteUrl(Text::HELP_URL);
      QDesktopServices::openUrl(QUrl(siteUrl));
    }

    virtual void VisitSite()
    {
      const QLatin1String siteUrl(Text::PROGRAM_SITE);
      QDesktopServices::openUrl(QUrl(siteUrl));
    }

    virtual void VisitFAQ()
    {
      const QLatin1String faqUrl(Text::FAQ_URL);
      QDesktopServices::openUrl(QUrl(faqUrl));
    }

    virtual void ReportIssue()
    {
      const QLatin1String faqUrl(Text::REPORT_BUG_URL);
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
    
    virtual bool event(QEvent* event)
    {
      const bool res = ::MainWindow::event(event);
      if (event && QEvent::LanguageChange == event->type())
      {
        FillLayoutMenu();
      }
      return res;
    }

    virtual void changeEvent(QEvent* event)
    {
      if (event && QEvent::LanguageChange == event->type())
      {
        Dbg("Retranslate main UI");
        retranslateUi(this);
      }
      ::MainWindow::changeEvent(event);
    }
  private:
    typedef std::pair<QWidget*, QToolBar*> WidgetOnToolbar;

    WidgetOnToolbar AddWidgetOnToolbar(QWidget* widget, bool lastInRow)
    {
      QToolBar* const toolBar = new QToolBar(this);
      toolBar->setObjectName(widget->objectName());
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
      return WidgetOnToolbar(widget, toolBar);
    }

    QWidget* AddWidgetOnLayout(QWidget* widget)
    {
      centralWidget()->layout()->addWidget(widget);
      return widget;
    }
    
    void FillLayoutMenu()
    {
      menuLayout->clear();
      for (std::vector<WidgetOnToolbar>::const_iterator it = Toolbars.begin(), lim = Toolbars.end(); it != lim; ++it)
      {
        it->second->setWindowTitle(it->first->windowTitle());
        menuLayout->addAction(it->second->toggleViewAction());
      }
    }
  private:
    const Parameters::Container::Ptr Options;
    const UI::Language::Ptr Language;
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
    std::vector<WidgetOnToolbar> Toolbars;
  };
}

QPointer<MainWindow> MainWindow::Create(Parameters::Container::Ptr options, const Strings::Array& cmdline)
{
  QPointer<MainWindow> res(new MainWindowImpl(options, cmdline));
  res->show();
  return res;
}
