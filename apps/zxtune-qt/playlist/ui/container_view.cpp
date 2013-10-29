/**
* 
* @file
*
* @brief Playlist container view implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "container_view.h"
#include "container_view.ui.h"
#include "playlist_view.h"
#include "playlist/io/export.h"
#include "playlist/parameters.h"
#include "playlist/supp/controller.h"
#include "playlist/supp/container.h"
#include "playlist/supp/scanner.h"
#include "playlist/supp/session.h"
#include "ui/utils.h"
#include "ui/tools/filedialog.h"
#include "ui/tools/parameters_helpers.h"
//common includes
#include <contract.h>
#include <error.h>
//library includes
#include <debug/log.h>
//std includes
#include <cassert>
//qt includes
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QMenu>

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

    virtual bool IsValid() const
    {
      return Current.get() != 0;
    }

    virtual Playlist::Controller::Ptr Get() const
    {
      assert(IsValid());
      return Current;
    }

    virtual void Next()
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
    for (QFileInfoList::const_iterator it = files.begin(), lim = files.end(); it != lim; ++it)
    {
      container.OpenPlaylist(it->absoluteFilePath());
    }
  }

  class ContainerViewImpl : public Playlist::UI::ContainerView
                          , public Playlist::UI::Ui_ContainerView
  {
  public:
    ContainerViewImpl(QWidget& parent, Parameters::Container::Ptr parameters)
      : Playlist::UI::ContainerView(parent)
      , Options(parameters)
      , Container(Playlist::Container::Create(parameters))
      , Session(Playlist::Session::Create())
      , ActionsMenu(new QMenu(this))
      , ActivePlaylistView(0)
    {
      //setup self
      setupUi(this);
      SetupMenu();

      //playlist actions
      Require(connect(actionCreatePlaylist, SIGNAL(triggered()), SLOT(CreatePlaylist())));
      Require(connect(actionLoadPlaylist, SIGNAL(triggered()), SLOT(LoadPlaylist())));
      Require(connect(actionSavePlaylist, SIGNAL(triggered()), SLOT(SavePlaylist())));
      Require(connect(actionRenamePlaylist, SIGNAL(triggered()), SLOT(RenamePlaylist())));
      Require(connect(actionClosePlaylist, SIGNAL(triggered()), SLOT(CloseCurrentPlaylist())));
      Require(connect(actionClearPlaylist, SIGNAL(triggered()), SLOT(Clear())));

      Require(connect(Container.get(), SIGNAL(PlaylistCreated(Playlist::Controller::Ptr)),
        SLOT(CreatePlaylist(Playlist::Controller::Ptr))));

      Require(connect(widgetsContainer, SIGNAL(tabCloseRequested(int)), SLOT(ClosePlaylist(int))));

      Parameters::BooleanValue::Bind(*actionLoop, *Options, Parameters::ZXTuneQT::Playlist::LOOPED, Parameters::ZXTuneQT::Playlist::LOOPED_DEFAULT);
      Parameters::BooleanValue::Bind(*actionRandom, *Options, Parameters::ZXTuneQT::Playlist::RANDOMIZED, Parameters::ZXTuneQT::Playlist::RANDOMIZED_DEFAULT);

      Dbg("Created at %1%", this);
    }

    virtual ~ContainerViewImpl()
    {
      Dbg("Destroyed at %1%", this);
    }

    virtual void Setup(const QStringList& items)
    {
      if (items.empty())
      {
        if (Session->Empty())
        {
          LoadDefaultPlaylists(*Container);
        }
        else
        {
          Session->Load(Container);
        }
      }
      else
      {
        Playlist::UI::View& pl = CreateAnonymousPlaylist();
        pl.AddItems(items);
      }
      if (widgetsContainer->count() == 0)
      {
        CreateAnonymousPlaylist();
      }
    }

    virtual void Teardown()
    {
      Playlist::Controller::Iterator::Ptr iter(new PlaylistsIterator(*widgetsContainer));
      Session->Save(iter);
    }

    virtual QMenu* GetActionsMenu() const
    {
      return ActionsMenu;
    }

    virtual void Play()
    {
      Playlist::UI::View& pl = GetActivePlaylist();
      pl.Play();
    }

    virtual void Pause()
    {
      Playlist::UI::View& pl = GetActivePlaylist();
      pl.Pause();
    }

    virtual void Stop()
    {
      Playlist::UI::View& pl = GetActivePlaylist();
      pl.Stop();
    }

    virtual void Finish()
    {
      Playlist::UI::View& pl = GetActivePlaylist();
      pl.Finish();
    }

    virtual void Next()
    {
      Playlist::UI::View& pl = GetActivePlaylist();
      pl.Next();
    }

    virtual void Prev()
    {
      Playlist::UI::View& pl = GetActivePlaylist();
      pl.Prev();
    }

    virtual void Clear()
    {
      Playlist::UI::View& pl = GetVisiblePlaylist();
      pl.Clear();
    }

    virtual void AddFiles()
    {
      Playlist::UI::View& pl = GetVisiblePlaylist();
      pl.AddFiles();
    }

    virtual void AddFolder()
    {
      Playlist::UI::View& pl = GetVisiblePlaylist();
      pl.AddFolder();
    }

    virtual void CreatePlaylist()
    {
      CreateAnonymousPlaylist();
    }

    virtual void LoadPlaylist()
    {
      QString file;
      if (UI::OpenSingleFileDialog(actionLoadPlaylist->text(),
         Playlist::UI::ContainerView::tr("Playlist files (*.xspf *.ayl)"), file))
      {
        Container->OpenPlaylist(file);
      }
    }

    virtual void SavePlaylist()
    {
      Playlist::UI::View& pl = GetVisiblePlaylist();
      pl.Save();
    }

    virtual void RenamePlaylist()
    {
      Playlist::UI::View& pl = GetVisiblePlaylist();
      pl.Rename();
    }

    virtual void CloseCurrentPlaylist()
    {
      ClosePlaylist(widgetsContainer->currentIndex());
    }

    virtual void ClosePlaylist(int index)
    {
      Playlist::UI::View* const view = static_cast<Playlist::UI::View*>(widgetsContainer->widget(index));
      view->hide();//to save layout
      widgetsContainer->removeTab(index);
      Dbg("Closed playlist idx=%1% val=%2%, active=%3%",
        index, view, ActivePlaylistView);
      if (view == ActivePlaylistView)
      {
        ActivePlaylistView = 0;
        SwitchToLastPlaylist();
      }
      view->deleteLater();
    }

    //qwidget virtuals
    virtual void changeEvent(QEvent* event)
    {
      if (event && QEvent::LanguageChange == event->type())
      {
        Dbg("Retranslating UI");
        retranslateUi(this);
        SetMenuTitle();
      }
      Playlist::UI::ContainerView::changeEvent(event);
    }

    virtual void contextMenuEvent(QContextMenuEvent* event)
    {
      ActionsMenu->exec(event->globalPos());
    }

    virtual void mouseDoubleClickEvent(QMouseEvent* /*event*/)
    {
      CreatePlaylist();
    }
  private:
    virtual void CreatePlaylist(Playlist::Controller::Ptr ctrl)
    {
      RegisterPlaylist(ctrl);
    }

    virtual void RenamePlaylist(const QString& name)
    {
      if (QObject* sender = this->sender())
      {
        //assert(dynamic_cast<QWidget*>(sender));
        QWidget* const widget = static_cast<QWidget*>(sender);
        const int idx = widgetsContainer->indexOf(widget);
        if (idx != -1)
        {
          widgetsContainer->setTabText(idx, name);
        }
      }
    }

    virtual void ActivateItem(Playlist::Item::Data::Ptr item)
    {
      if (QObject* sender = this->sender())
      {
        //assert(dynamic_cast<Playlist::UI::View*>(sender));
        Playlist::UI::View* const newView = static_cast<Playlist::UI::View*>(sender);
        if (newView != ActivePlaylistView)
        {
          Dbg("Switched playlist %1% -> %2%", newView, ActivePlaylistView);
          ActivePlaylistView->Stop();
          ActivePlaylistView = newView;
        }
      }
      emit ItemActivated(item);
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

    Playlist::UI::View& RegisterPlaylist(Playlist::Controller::Ptr playlist)
    {
      Playlist::UI::View* const plView = Playlist::UI::View::Create(*this, playlist, Options);
      widgetsContainer->addTab(plView, playlist->GetName());
      Require(connect(plView, SIGNAL(Renamed(const QString&)), SLOT(RenamePlaylist(const QString&))));
      Require(connect(plView, SIGNAL(ItemActivated(Playlist::Item::Data::Ptr)), SLOT(ActivateItem(Playlist::Item::Data::Ptr))));
      if (!ActivePlaylistView)
      {
        ActivePlaylistView = plView;
      }
      widgetsContainer->setCurrentWidget(plView);
      return *plView;
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
        ActivePlaylistView = static_cast<Playlist::UI::View*>(widget);
        Dbg("Switching to playlist idx=%1% val=%2%", index, ActivePlaylistView);
      }
    }
  private:
    const Parameters::Container::Ptr Options;
    const Playlist::Container::Ptr Container;
    const Playlist::Session::Ptr Session;
    QMenu* const ActionsMenu;
    //state context
    Playlist::UI::View* ActivePlaylistView;
  };
}

namespace Playlist
{
  namespace UI
  {
    ContainerView::ContainerView(QWidget& parent) : QWidget(&parent)
    {
    }

    ContainerView* ContainerView::Create(QWidget& parent, Parameters::Container::Ptr parameters)
    {
      REGISTER_METATYPE(Playlist::Controller::Ptr);
      return new ContainerViewImpl(parent, parameters);
    }
  }
}
