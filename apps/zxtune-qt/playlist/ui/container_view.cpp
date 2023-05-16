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
      , Index(-1)
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
      if (Playlist::UI::View* view = static_cast<Playlist::UI::View*>(Container.widget(++Index)))
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
    int Index;
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
      , Options(parameters)
      , Container(Playlist::Container::Create(parameters))
      , Session(Playlist::Session::Create())
      , ActionsMenu(new QMenu(this))
      , ActivePlaylistView(nullptr)
    {
      // setup self
      setupUi(this);
      SetupMenu();

      // playlist actions
      Require(connect(actionCreatePlaylist, SIGNAL(triggered()), SLOT(CreatePlaylist())));
      Require(connect(actionLoadPlaylist, SIGNAL(triggered()), SLOT(LoadPlaylist())));
      Require(connect(actionSavePlaylist, SIGNAL(triggered()), SLOT(SavePlaylist())));
      Require(connect(actionRenamePlaylist, SIGNAL(triggered()), SLOT(RenamePlaylist())));
      Require(connect(actionClosePlaylist, SIGNAL(triggered()), SLOT(CloseCurrentPlaylist())));
      Require(connect(actionClearPlaylist, SIGNAL(triggered()), SLOT(Clear())));

      Require(connect(Container.get(), SIGNAL(PlaylistCreated(Playlist::Controller::Ptr)),
                      SLOT(CreatePlaylist(Playlist::Controller::Ptr))));

      Require(connect(widgetsContainer, SIGNAL(tabCloseRequested(int)), SLOT(ClosePlaylist(int))));

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
      Playlist::UI::View* const view = static_cast<Playlist::UI::View*>(widgetsContainer->widget(index));
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

    void CreatePlaylist(Playlist::Controller::Ptr ctrl) override
    {
      RegisterPlaylist(ctrl);
    }

    void RenamePlaylist(const QString& name) override
    {
      if (QObject* sender = this->sender())
      {
        // assert(dynamic_cast<QWidget*>(sender));
        QWidget* const widget = static_cast<QWidget*>(sender);
        const int idx = widgetsContainer->indexOf(widget);
        if (idx != -1)
        {
          widgetsContainer->setTabText(idx, name);
        }
      }
    }

    void ActivateItem(Playlist::Item::Data::Ptr /*item*/) override
    {
      if (QObject* sender = this->sender())
      {
        // assert(dynamic_cast<Playlist::UI::View*>(sender));
        Playlist::UI::View* const newView = static_cast<Playlist::UI::View*>(sender);
        if (newView != ActivePlaylistView)
        {
          ActivePlaylistView->Stop();  // just update state
          SwitchTo(newView);
        }
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
      const Playlist::Controller::Ptr pl = Container->CreatePlaylist(Playlist::UI::ContainerView::tr("Default"));
      return RegisterPlaylist(pl);
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
      Playlist::UI::View* const plView = Playlist::UI::View::Create(*this, playlist, Options);
      widgetsContainer->addTab(plView, playlist->GetName());
      Require(connect(plView, SIGNAL(Renamed(const QString&)), SLOT(RenamePlaylist(const QString&))));
      Require(connect(plView, SIGNAL(ItemActivated(Playlist::Item::Data::Ptr)),
                      SLOT(ActivateItem(Playlist::Item::Data::Ptr))));
      Require(connect(plView, SIGNAL(ItemActivated(Playlist::Item::Data::Ptr)),
                      SIGNAL(ItemActivated(Playlist::Item::Data::Ptr))));
      if (!ActivePlaylistView)
      {
        SwitchTo(plView);
      }
      widgetsContainer->setCurrentWidget(plView);
      return *plView;
    }

    void SwitchTo(Playlist::UI::View* plView)
    {
      Dbg("Switch playlist {} -> {}", static_cast<const void*>(ActivePlaylistView), static_cast<const void*>(plView));
      const bool wasPrevious = ActivePlaylistView != nullptr;
      if (wasPrevious)
      {
        const Playlist::Item::Iterator::Ptr iter = ActivePlaylistView->GetPlaylist()->GetIterator();
        Require(iter->disconnect(this, SIGNAL(Activated(Playlist::Item::Data::Ptr))));
        Require(iter->disconnect(this, SIGNAL(Deactivated())));
      }
      ActivePlaylistView = plView;
      if (ActivePlaylistView)
      {
        const Playlist::Controller::Ptr ctrl = ActivePlaylistView->GetPlaylist();
        const Playlist::Item::Iterator::Ptr iter = ctrl->GetIterator();
        Require(
            connect(iter, SIGNAL(Activated(Playlist::Item::Data::Ptr)), SIGNAL(Activated(Playlist::Item::Data::Ptr))));
        Require(connect(iter, SIGNAL(Deactivated()), SIGNAL(Deactivated())));
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
      if (Playlist::UI::View* view = static_cast<Playlist::UI::View*>(widgetsContainer->currentWidget()))
      {
        return *view;
      }
      return GetActivePlaylist();
    }

    void SwitchToLastPlaylist()
    {
      Dbg("Move to another playlist");
      if (int total = widgetsContainer->count())
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
      Parameters::IntType idx = 0, trk = 0;
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
        QTimer::singleShot(1000, pl.GetPlaylist()->GetIterator(), SLOT(Reset()));
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
    Playlist::UI::View* ActivePlaylistView;
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
    return new ContainerViewImpl(parent, parameters);
  }
}  // namespace Playlist::UI
