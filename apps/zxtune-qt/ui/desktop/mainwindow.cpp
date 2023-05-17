/**
 *
 * @file
 *
 * @brief Main window implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "mainwindow.h"
#include "language.h"
#include "mainwindow.ui.h"
#include "playlist/ui/container_view.h"
#include "supp/playback_supp.h"
#include "ui/controls/analyzer_control.h"
#include "ui/controls/playback_controls.h"
#include "ui/controls/playback_options.h"
#include "ui/controls/seek_controls.h"
#include "ui/controls/status_control.h"
#include "ui/controls/volume_control.h"
#include "ui/format.h"
#include "ui/informational/aboutdialog.h"
#include "ui/informational/componentsdialog.h"
#include "ui/parameters.h"
#include "ui/preferences/preferencesdialog.h"
#include "ui/state.h"
#include "ui/tools/errordialog.h"
#include "ui/utils.h"
#include "update/check.h"
#include "urls.h"
// common includes
#include <contract.h>
// library includes
#include <debug/log.h>
#include <platform/version/api.h>
#include <strings/format.h>
// std includes
#include <utility>
// qt includes
#include <QtCore/QUrl>
#include <QtGui/QCloseEvent>
#include <QtGui/QDesktopServices>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QToolBar>

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

  class DesktopMainWindowImpl
    : public DesktopMainWindow
    , public Ui::MainWindow
  {
  public:
    explicit DesktopMainWindowImpl(Parameters::Container::Ptr options)
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
      // fill menu
      menubar->addMenu(Controls->GetActionsMenu());
      menubar->addMenu(MultiPlaylist->GetActionsMenu());
      menubar->addMenu(menuHelp);
      // fill toolbar and layout menu
      {
        Toolbars.push_back(AddWidgetOnToolbar(Controls, false));
        Toolbars.push_back(AddWidgetOnToolbar(FastOptions, false));
        Toolbars.push_back(AddWidgetOnToolbar(Volume, true));
        Toolbars.push_back(AddWidgetOnToolbar(Status, false));
        Toolbars.push_back(AddWidgetOnToolbar(Seeking, true));
        Toolbars.push_back(AddWidgetOnToolbar(Analyzer, false));
        // playlist is mandatory and cannot be hidden
        AddWidgetOnLayout(MultiPlaylist);
        State->Load();
        FillLayoutMenu();
      }

      // connect root actions
      Require(connect(actionComponents, SIGNAL(triggered()), SLOT(ShowComponentsInformation())));
      Require(connect(actionAbout, SIGNAL(triggered()), SLOT(ShowAboutProgram())));
      Require(connect(actionOnlineHelp, SIGNAL(triggered()), SLOT(VisitHelp())));
      Require(connect(actionWebSite, SIGNAL(triggered()), SLOT(VisitSite())));
      Require(connect(actionOnlineFAQ, SIGNAL(triggered()), SLOT(VisitFAQ())));
      Require(connect(actionReportBug, SIGNAL(triggered()), SLOT(ReportIssue())));
      Require(connect(actionAboutQt, SIGNAL(triggered()), SLOT(ShowAboutQt())));
      Require(connect(actionPreferences, SIGNAL(triggered()), SLOT(ShowPreferences())));
      if (Update::CheckOperation* op = Update::CheckOperation::Create(*this))
      {
        Require(op->connect(actionCheckUpdates, SIGNAL(triggered()), SLOT(Execute())));
        Require(connect(op, SIGNAL(ErrorOccurred(const Error&)), SLOT(ShowError(const Error&))));
      }
      else
      {
        actionCheckUpdates->setEnabled(false);
      }

      Require(MultiPlaylist->connect(Controls, SIGNAL(OnPrevious()), SLOT(Prev())));
      Require(MultiPlaylist->connect(Controls, SIGNAL(OnNext()), SLOT(Next())));
      Require(MultiPlaylist->connect(Playback, SIGNAL(OnStartModule(Sound::Backend::Ptr, Playlist::Item::Data::Ptr)),
                                     SLOT(Play())));
      Require(MultiPlaylist->connect(Playback, SIGNAL(OnResumeModule()), SLOT(Play())));
      Require(MultiPlaylist->connect(Playback, SIGNAL(OnPauseModule()), SLOT(Pause())));
      Require(MultiPlaylist->connect(Playback, SIGNAL(OnStopModule()), SLOT(Stop())));
      Require(MultiPlaylist->connect(Playback, SIGNAL(OnFinishModule()), SLOT(Finish())));
      Require(Playback->connect(MultiPlaylist, SIGNAL(Activated(Playlist::Item::Data::Ptr)),
                                SLOT(SetDefaultItem(Playlist::Item::Data::Ptr))));
      Require(Playback->connect(MultiPlaylist, SIGNAL(ItemActivated(Playlist::Item::Data::Ptr)),
                                SLOT(SetItem(Playlist::Item::Data::Ptr))));
      Require(Playback->connect(MultiPlaylist, SIGNAL(Deactivated()), SLOT(ResetItem())));
      Require(connect(Playback, SIGNAL(OnStartModule(Sound::Backend::Ptr, Playlist::Item::Data::Ptr)),
                      SLOT(StartModule(Sound::Backend::Ptr, Playlist::Item::Data::Ptr))));
      Require(connect(Playback, SIGNAL(OnStopModule()), SLOT(StopModule())));
      Require(connect(Playback, SIGNAL(ErrorOccurred(const Error&)), SLOT(ShowError(const Error&))));
      Require(connect(actionAddFiles, SIGNAL(triggered()), MultiPlaylist, SLOT(AddFiles())));
      Require(connect(actionAddFolder, SIGNAL(triggered()), MultiPlaylist, SLOT(AddFolder())));

      StopModule();

      MultiPlaylist->Setup();
    }

    void SetCmdline(const QStringList& args) override
    {
      if (this->sender())
      {
        setWindowState((windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
        raise();
        activateWindow();
      }
      if (!args.empty())
      {
        MultiPlaylist->Open(args);
      }
    }

    void StartModule(Sound::Backend::Ptr /*player*/, Playlist::Item::Data::Ptr item) override
    {
      setWindowTitle(
          ToQString(Strings::Format("{1} [{0}]", Platform::Version::GetProgramTitle(), item->GetDisplayName())));
      Playing = true;
    }

    void StopModule() override
    {
      Playing = false;
      setWindowTitle(ToQString(Platform::Version::GetProgramTitle()));
    }

    void ShowPreferences() override
    {
      UI::ShowPreferencesDialog(*this, Playing);
    }

    void ShowComponentsInformation() override
    {
      UI::ShowComponentsInformation(*this);
    }

    void ShowAboutProgram() override
    {
      UI::ShowProgramInformation(*this);
    }

    void ShowAboutQt() override
    {
      QMessageBox::aboutQt(this);
    }

    void VisitHelp() override
    {
      QDesktopServices::openUrl(ToQString(Urls::Help()));
    }

    void VisitSite() override
    {
      QDesktopServices::openUrl(ToQString(Urls::Site()));
    }

    void VisitFAQ() override
    {
      QDesktopServices::openUrl(ToQString(Urls::Faq()));
    }

    void ReportIssue() override
    {
      QDesktopServices::openUrl(ToQString(Urls::Bugreport()));
    }

    void ShowError(const Error& err) override
    {
      ShowErrorMessage(QString(), err);
    }

    // QWidgets virtuals
    void closeEvent(QCloseEvent* event) override
    {
      Playback->Stop();
      State->Save();
      MultiPlaylist->Teardown();
      event->accept();
    }

    bool event(QEvent* event) override
    {
      const bool res = ::MainWindow::event(event);
      if (event && QEvent::LanguageChange == event->type())
      {
        FillLayoutMenu();
      }
      return res;
    }

    void changeEvent(QEvent* event) override
    {
      if (event && QEvent::LanguageChange == event->type())
      {
        Dbg("Retranslate main UI");
        retranslateUi(this);
      }
      ::MainWindow::changeEvent(event);
    }

  private:
    using WidgetOnToolbar = std::pair<QWidget*, QToolBar*>;

    WidgetOnToolbar AddWidgetOnToolbar(QWidget* widget, bool lastInRow)
    {
      const auto toolBar = new QToolBar(this);
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
      for (const auto& toolbar : Toolbars)
      {
        toolbar.second->setWindowTitle(toolbar.first->windowTitle());
        menuLayout->addAction(toolbar.second->toggleViewAction());
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
}  // namespace

MainWindow::Ptr DesktopMainWindow::Create(Parameters::Container::Ptr options)
{
  const MainWindow::Ptr res = new DesktopMainWindowImpl(options);
  res->show();
  return res;
}
