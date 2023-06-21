/**
 *
 * @file
 *
 * @brief Playlist container view implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "container_view.h"
#include "container_view.ui.h"
#include "playlist/io/export.h"
#include "playlist/parameters.h"
#include "playlist/supp/container.h"
#include "playlist/supp/controller.h"
#include "playlist/supp/scanner.h"
#include "playlist/supp/session.h"
#include "playlist_view.h"
#include "ui/tools/filedialog.h"
#include "ui/tools/parameters_helpers.h"
#include "ui/utils.h"
// common includes
#include <contract.h>
#include <error.h>
#include <make_ptr.h>
// library includes
#include <debug/log.h>
// std includes
#include <cassert>
#include <utility>
// qt includes
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QTimer>
#include <QtGui/QContextMenuEvent>
#include <QtWidgets/QMenu>

namespace
{
  const Debug::Stream Dbg("Playlist::UI::ContainerView");

  class PlaylistsIterator : public Playlist::Controller::Iterator
  {
  public:
    explicit PlaylistsIterator(QTabWidget& ctr)
      : Container(ctr)
    {
      GetNext();
    }

    bool IsValid() const override
    {
      return Current.get() != nullptr;
    }

    Playlist::Controller::Ptr Get() const override
    {
      assert(IsValid());
      return Current;
    }

    void Next() override
    {
      assert(IsValid());
      GetNext();
    }

  private:
    void GetNext()
    {
      if (auto* view = static_cast<Playlist::UI::View*>(Container.widget(++Index)))
      {
        Current = view->GetPlaylist();
      }
      else
      {
        Current = Playlist::Controller::Ptr();
      }
    }

  private:
    QTabWidget& Container;
    int Index = -1;
    Playlist::Controller::Ptr Current;
  };

  void LoadDefaultPlaylists(Playlist::Container& container)
  {
    const QDir appDir(QCoreApplication::applicationDirPath());
    const QFileInfoList files = appDir.entryInfoList(QStringList("*.xspf"), QDir::Files | QDir::Readable, QDir::Name);
    for (const auto& file : files)
    {
      container.OpenPlaylist(file.absoluteFilePath());
    }
  }

  class ContainerViewImpl
    : public Playlist::UI::ContainerView
    , public Playlist::UI::Ui_ContainerView
  {
  public:
    ContainerViewImpl(QWidget& parent, Parameters::Container::Ptr parameters)
      : Playlist::UI::ContainerView(parent)
      , Options(std::move(parameters))
      , Container(Playlist::Container::Create(Options))
      , Session(Playlist::Session::Create())
      , ActionsMenu(new QMenu(this))
    {
      // setup self
      setupUi(this);
      SetupMenu();

      // playlist actions
      Require(connect(actionCreatePlaylist, &QAction::triggered, this, &Playlist::UI::ContainerView::CreatePlaylist));
      Require(connect(actionLoadPlaylist, &QAction::triggered, this, &Playlist::UI::ContainerView::LoadPlaylist));
      Require(connect(actionSavePlaylist, &QAction::triggered, this, &Playlist::UI::ContainerView::SavePlaylist));
      Require(connect(actionRenamePlaylist, &QAction::triggered, this, &Playlist::UI::ContainerView::RenamePlaylist));
      Require(
          connect(actionClosePlaylist, &QAction::triggered, this, &Playlist::UI::ContainerView::CloseCurrentPlaylist));
      Require(connect(actionClearPlaylist, &QAction::triggered, this, &Playlist::UI::ContainerView::Clear));

      Require(connect(Container.get(), &Playlist::Container::PlaylistCreated, this,
                      [this](Playlist::Controller::Ptr ctrl) { RegisterPlaylist(std::move(ctrl)); }));
      Require(
          connect(widgetsContainer, &QTabWidget::tabCloseRequested, this, &Playlist::UI::ContainerView::ClosePlaylist));

      Parameters::BooleanValue::Bind(*actionLoop, *Options, Parameters::ZXTuneQT::Playlist::LOOPED,
                                     Parameters::ZXTuneQT::Playlist::LOOPED_DEFAULT);
      Parameters::BooleanValue::Bind(*actionRandom, *Options, Parameters::ZXTuneQT::Playlist::RANDOMIZED,
                                     Parameters::ZXTuneQT::Playlist::RANDOMIZED_DEFAULT);

      Dbg("Created at {}", Self());
    }

    ~ContainerViewImpl() override
    {
      Dbg("Destroyed at {}", Self());
    }

    void Setup() override
    {
      if (Session->Empty())
      {
        LoadDefaultPlaylists(*Container);
      }
      else
      {
        RestorePlaylistSession();
      }
      if (widgetsContainer->count() == 0)
      {
        CreateAnonymousPlaylist();
      }
    }

    void Teardown() override
    {
      for (PlaylistsIterator it(*widgetsContainer); it.IsValid(); it.Next())
      {
        it.Get()->Shutdown();
      }
      StorePlaylistSession();
    }

    void Open(const QStringList& items) override
    {
      auto& pl = GetCmdlineTarget();
      pl.AddItems(items);
    }

    QMenu* GetActionsMenu() const override
    {
      return ActionsMenu;
    }

    void Play() override
    {
      Playlist::UI::View& pl = GetActivePlaylist();
      pl.Play();
    }

    void Pause() override
    {
      Playlist::UI::View& pl = GetActivePlaylist();
      pl.Pause();
    }

    void Stop() override
    {
      Playlist::UI::View& pl = GetActivePlaylist();
      pl.Stop();
    }

    void Finish() override
    {
      Playlist::UI::View& pl = GetActivePlaylist();
      pl.Finish();
    }

    void Next() override
    {
      Playlist::UI::View& pl = GetActivePlaylist();
      pl.Next();
    }

    void Prev() override
    {
      Playlist::UI::View& pl = GetActivePlaylist();
      pl.Prev();
    }

    void Clear() override
    {
      Playlist::UI::View& pl = GetVisiblePlaylist();
      pl.Clear();
    }

    void AddFiles() override
    {
      Playlist::UI::View& pl = GetVisiblePlaylist();
      pl.AddFiles();
    }

    void AddFolder() override
    {
      Playlist::UI::View& pl = GetVisiblePlaylist();
      pl.AddFolder();
    }

    void CreatePlaylist() override
    {
      CreateAnonymousPlaylist();
    }

    void LoadPlaylist() override
    {
      QString file;
      if (UI::OpenSingleFileDialog(actionLoadPlaylist->text(),
                                   Playlist::UI::ContainerView::tr("Playlist files (*.xspf *.ayl)"), file))
      {
        Container->OpenPlaylist(file);
      }
    }

    void SavePlaylist() override
    {
      Playlist::UI::View& pl = GetVisiblePlaylist();
      pl.Save();
    }

    void RenamePlaylist() override
    {
      Playlist::UI::View& pl = GetVisiblePlaylist();
      pl.Rename();
    }

    void CloseCurrentPlaylist() override
    {
      ClosePlaylist(widgetsContainer->currentIndex());
    }

    void ClosePlaylist(int index) override
    {
      auto* const view = static_cast<Playlist::UI::View*>(widgetsContainer->widget(index));
      view->hide();  // to save layout
      view->GetPlaylist()->Shutdown();
      widgetsContainer->removeTab(index);
      Dbg("Closed playlist idx={} val={}, active={}", index, static_cast<const void*>(view),
          static_cast<const void*>(ActivePlaylistView));
      if (view == ActivePlaylistView)
      {
        emit Deactivated();
        ActivePlaylistView = nullptr;
        SwitchToLastPlaylist();
      }
      view->deleteLater();
    }

    // qwidget virtuals
    void changeEvent(QEvent* event) override
    {
      if (event && QEvent::LanguageChange == event->type())
      {
        Dbg("Retranslating UI");
        retranslateUi(this);
        SetMenuTitle();
      }
      Playlist::UI::ContainerView::changeEvent(event);
    }

    void contextMenuEvent(QContextMenuEvent* event) override
    {
      ActionsMenu->exec(event->globalPos());
    }

    void mouseDoubleClickEvent(QMouseEvent* /*event*/) override
    {
      CreatePlaylist();
    }

  private:
    const void* Self() const
    {
      return this;
    }

    void RenamePlaylist(QWidget* widget, const QString& name)
    {
      const int idx = widgetsContainer->indexOf(widget);
      if (idx != -1)
      {
        widgetsContainer->setTabText(idx, name);
      }
    }

  private:
    void SetupMenu()
    {
      SetMenuTitle();
      ActionsMenu->addAction(actionCreatePlaylist);
      ActionsMenu->addAction(actionLoadPlaylist);
      ActionsMenu->addAction(actionSavePlaylist);
      ActionsMenu->addAction(actionRenamePlaylist);
      ActionsMenu->addAction(actionClosePlaylist);
      ActionsMenu->addAction(actionClearPlaylist);
      ActionsMenu->addSeparator();
      ActionsMenu->addAction(actionLoop);
      ActionsMenu->addAction(actionRandom);
    }

    void SetMenuTitle()
    {
      ActionsMenu->setTitle(windowTitle());
    }

    Playlist::UI::View& CreateAnonymousPlaylist()
    {
      Dbg("Create default playlist");
      auto pl = Container->CreatePlaylist(Playlist::UI::ContainerView::tr("Default"));
      return RegisterPlaylist(std::move(pl));
    }

    Playlist::UI::View& GetEmptyPlaylist()
    {
      if (widgetsContainer->count() == 1 && 0 == ActivePlaylistView->GetPlaylist()->GetModel()->CountItems())
      {
        return *ActivePlaylistView;
      }
      else
      {
        return CreateAnonymousPlaylist();
      }
    }

    Playlist::UI::View& RegisterPlaylist(Playlist::Controller::Ptr playlist)
    {
      Playlist::UI::View* const plView = Playlist::UI::View::Create(*this, std::move(playlist), Options);
      widgetsContainer->addTab(plView, plView->GetPlaylist()->GetName());
      Require(connect(
          plView, &Playlist::UI::View::Renamed, this,
          [plView, this](const QString& name) { RenamePlaylist(plView, name); }, Qt::DirectConnection));
      Require(connect(
          plView, &Playlist::UI::View::ItemActivated, this, [plView, this]() { SwitchTo(plView); },
          Qt::DirectConnection));
      Require(connect(plView, &Playlist::UI::View::ItemActivated, this, &Playlist::UI::ContainerView::ItemActivated));
      if (!ActivePlaylistView)
      {
        SwitchTo(plView);
      }
      widgetsContainer->setCurrentWidget(plView);
      return *plView;
    }

    void SwitchTo(Playlist::UI::View* plView)
    {
      if (plView == ActivePlaylistView)
      {
        return;
      }
      Dbg("Switch playlist {} -> {}", static_cast<const void*>(ActivePlaylistView), static_cast<const void*>(plView));
      const bool wasPrevious = ActivePlaylistView != nullptr;
      if (wasPrevious)
      {
        ActivePlaylistView->Stop();  // just update state
        const Playlist::Item::Iterator::Ptr iter = ActivePlaylistView->GetPlaylist()->GetIterator();
        Require(disconnect(iter, nullptr, this, nullptr));
      }
      ActivePlaylistView = plView;
      if (ActivePlaylistView)
      {
        const Playlist::Controller::Ptr ctrl = ActivePlaylistView->GetPlaylist();
        const Playlist::Item::Iterator::Ptr iter = ctrl->GetIterator();
        Require(connect(iter, &Playlist::Item::Iterator::Activated, this, &Playlist::UI::ContainerView::Activated));
        Require(connect(iter, &Playlist::Item::Iterator::Deactivated, this, &Playlist::UI::ContainerView::Deactivated));
      }
    }

    Playlist::UI::View& GetActivePlaylist()
    {
      if (!ActivePlaylistView)
      {
        return CreateAnonymousPlaylist();
      }
      return *ActivePlaylistView;
    }

    Playlist::UI::View& GetVisiblePlaylist()
    {
      if (auto* view = static_cast<Playlist::UI::View*>(widgetsContainer->currentWidget()))
      {
        return *view;
      }
      return GetActivePlaylist();
    }

    void SwitchToLastPlaylist()
    {
      Dbg("Move to another playlist");
      if (const auto total = widgetsContainer->count())
      {
        ActivatePlaylist(total - 1);
      }
      else
      {
        CreateAnonymousPlaylist();
      }
    }

    void ActivatePlaylist(int index)
    {
      if (QWidget* widget = widgetsContainer->widget(index))
      {
        SwitchTo(static_cast<Playlist::UI::View*>(widget));
      }
    }

    void RestorePlaylistSession()
    {
      Session->Load(Container);
      Parameters::IntType idx = 0;
      Parameters::IntType trk = 0;
      Options->FindValue(Parameters::ZXTuneQT::Playlist::INDEX, idx);
      Options->FindValue(Parameters::ZXTuneQT::Playlist::TRACK, trk);
      Dbg("Restore current playlist {} with track {}", idx, trk);
      ActivatePlaylist(idx);
      widgetsContainer->setCurrentIndex(idx);
      const Playlist::Controller::Ptr playlist = ActivePlaylistView->GetPlaylist();
      playlist->GetModel()->WaitOperationFinish();
      playlist->GetIterator()->Select(trk);
    }

    void StorePlaylistSession()
    {
      const Playlist::Controller::Iterator::Ptr iter = MakePtr<PlaylistsIterator>(*widgetsContainer);
      Session->Save(iter);
      const uint_t idx = widgetsContainer->indexOf(ActivePlaylistView);
      const uint_t trk = ActivePlaylistView->GetPlaylist()->GetIterator()->GetIndex();
      Dbg("Store current playlist {} (visible is {}), track {}", idx, widgetsContainer->currentIndex(), trk);
      Options->SetValue(Parameters::ZXTuneQT::Playlist::INDEX, idx);
      Options->SetValue(Parameters::ZXTuneQT::Playlist::TRACK, trk);
    }

    Playlist::UI::View& GetCmdlineTarget()
    {
      using namespace Parameters::ZXTuneQT::Playlist;
      Parameters::IntType target = CMDLINE_TARGET_DEFAULT;
      Options->FindValue(CMDLINE_TARGET, target);
      switch (target)
      {
      case CMDLINE_TARGET_ACTIVE:
        return GetActivePlaylist();
      case CMDLINE_TARGET_VISIBLE:
        return GetVisiblePlaylist();
      default:
      {
        auto& pl = GetEmptyPlaylist();
        QTimer::singleShot(1000, pl.GetPlaylist()->GetIterator(), qOverload<>(&Playlist::Item::Iterator::Reset));
        return pl;
      }
      }
    }

  private:
    const Parameters::Container::Ptr Options;
    const Playlist::Container::Ptr Container;
    const Playlist::Session::Ptr Session;
    QMenu* const ActionsMenu;
    // state context
    Playlist::UI::View* ActivePlaylistView = nullptr;
  };
}  // namespace

namespace Playlist::UI
{
  ContainerView::ContainerView(QWidget& parent)
    : QWidget(&parent)
  {}

  ContainerView* ContainerView::Create(QWidget& parent, Parameters::Container::Ptr parameters)
  {
    REGISTER_METATYPE(Playlist::Controller::Ptr);
    return new ContainerViewImpl(parent, std::move(parameters));
  }
}  // namespace Playlist::UI
