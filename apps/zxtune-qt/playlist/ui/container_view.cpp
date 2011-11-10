/*
Abstract:
  Playlist container view implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "container_view.h"
#include "container_view.ui.h"
#include "playlist_view.h"
#include "playlist/io/export.h"
#include "playlist/playlist_parameters.h"
#include "playlist/supp/controller.h"
#include "playlist/supp/container.h"
#include "playlist/supp/scanner.h"
#include "ui/utils.h"
#include "ui/tools/filedialog.h"
#include "ui/tools/parameters_helpers.h"
//common includes
#include <error.h>
#include <logging.h>
//std includes
#include <cassert>
//qt includes
#include <QtGui/QContextMenuEvent>
#include <QtGui/QMenu>

namespace
{
  const std::string THIS_MODULE("Playlist::UI::ContainerView");

  class ContainerViewImpl : public Playlist::UI::ContainerView
                          , public Ui::PlaylistContainerView
  {
  public:
    ContainerViewImpl(QWidget& parent, Parameters::Container::Ptr parameters)
      : Playlist::UI::ContainerView(parent)
      , Options(parameters)
      , Container(Playlist::Container::Create(*this, parameters))
      , ActionsMenu(new QMenu(tr("Playlist"), this))
      , ActivePlaylistView(0)
    {
      //setup self
      setupUi(this);
      SetupMenu();

      //connect actions
      this->connect(actionAddFiles, SIGNAL(triggered()), SLOT(AddFiles()));
      this->connect(actionAddFolder, SIGNAL(triggered()), SLOT(AddFolder()));
      //playlist actions
      this->connect(actionCreatePlaylist, SIGNAL(triggered()), SLOT(CreatePlaylist()));
      this->connect(actionLoadPlaylist, SIGNAL(triggered()), SLOT(LoadPlaylist()));
      this->connect(actionSavePlaylist, SIGNAL(triggered()), SLOT(SavePlaylist()));
      this->connect(actionRenamePlaylist, SIGNAL(triggered()), SLOT(RenamePlaylist()));
      this->connect(actionClosePlaylist, SIGNAL(triggered()), SLOT(CloseCurrentPlaylist()));
      this->connect(actionClearPlaylist, SIGNAL(triggered()), SLOT(Clear()));

      this->connect(widgetsContainer, SIGNAL(tabCloseRequested(int)), SLOT(ClosePlaylist(int)));

      Parameters::BooleanValue::Bind(*actionDeepScan, *Options, Parameters::ZXTuneQT::Playlist::DEEP_SCANNING, Parameters::ZXTuneQT::Playlist::DEEP_SCANNING_DEFAULT);
      Parameters::BooleanValue::Bind(*actionLoop, *Options, Parameters::ZXTuneQT::Playlist::LOOPED, Parameters::ZXTuneQT::Playlist::LOOPED_DEFAULT);
      Parameters::BooleanValue::Bind(*actionRandom, *Options, Parameters::ZXTuneQT::Playlist::RANDOMIZED, Parameters::ZXTuneQT::Playlist::RANDOMIZED_DEFAULT);

      Log::Debug(THIS_MODULE, "Created at %1%", this);
    }

    virtual ~ContainerViewImpl()
    {
      Log::Debug(THIS_MODULE, "Destroyed at %1%", this);
    }

    virtual void CreatePlaylist(const QStringList& items)
    {
      Playlist::UI::View& pl = CreateAnonymousPlaylist();
      pl.AddItems(items);
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
      QStringList files;
      if (FileDialog::Instance().OpenMultipleFiles(actionAddFiles->text(),
        tr("All files (*.*)"), files))
      {
        AddItemsToVisiblePlaylist(files);
      }
    }

    virtual void AddFolder()
    {
      QStringList folders;
      folders += QString();
      if (FileDialog::Instance().OpenFolder(actionAddFolder->text(), folders.front()))
      {
        AddItemsToVisiblePlaylist(folders);
      }
    }

    virtual void CreatePlaylist()
    {
      CreateAnonymousPlaylist();
    }

    virtual void LoadPlaylist()
    {
      QString file;
      if (FileDialog::Instance().OpenSingleFile(actionLoadPlaylist->text(),
         tr("Playlist files (*.xspf *.ayl)"), file))
      {
        if (Playlist::Controller::Ptr pl = Container->OpenPlaylist(file))
        {
          RegisterPlaylist(pl);
        }
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
      const Playlist::Controller& controller = pl.GetPlaylist();
      widgetsContainer->setTabText(widgetsContainer->currentIndex(), controller.GetName());
    }

    virtual void CloseCurrentPlaylist()
    {
      ClosePlaylist(widgetsContainer->currentIndex());
    }

    virtual void ClosePlaylist(int index)
    {
      Playlist::UI::View* const view = static_cast<Playlist::UI::View*>(widgetsContainer->widget(index));
      widgetsContainer->removeTab(index);
      Log::Debug(THIS_MODULE, "Closed playlist idx=%1% val=%2%, active=%3%",
        index, view, ActivePlaylistView);
      if (view == ActivePlaylistView)
      {
        ActivePlaylistView = 0;
        SwitchToLastPlaylist();
      }
      view->deleteLater();
    }

    //qwidget virtuals
    virtual void contextMenuEvent(QContextMenuEvent* event)
    {
      ActionsMenu->exec(event->globalPos());
    }
  private:
    void PlaylistItemActivated(const Playlist::Item::Data& item)
    {
      if (QObject* sender = this->sender())
      {
        //assert(dynamic_cast<Playlist::UI::View*>(sender));
        Playlist::UI::View* const newView = static_cast<Playlist::UI::View*>(sender);
        if (newView != ActivePlaylistView)
        {
          Log::Debug(THIS_MODULE, "Switched playlist %1% -> %2%", newView, ActivePlaylistView);
          ActivePlaylistView->Stop();
          ActivePlaylistView = newView;
        }
      }
      OnItemActivated(item);
    }
  private:
    void SetupMenu()
    {
      ActionsMenu->addAction(actionAddFiles);
      ActionsMenu->addAction(actionAddFolder);
      ActionsMenu->addAction(actionDeepScan);
      ActionsMenu->addSeparator();
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

    Playlist::UI::View& CreateAnonymousPlaylist()
    {
      Log::Debug(THIS_MODULE, "Create default playlist");
      const Playlist::Controller::Ptr pl = Container->CreatePlaylist(tr("Default"));
      return RegisterPlaylist(pl);
    }

    Playlist::UI::View& RegisterPlaylist(Playlist::Controller::Ptr playlist)
    {
      Playlist::UI::View* const plView = Playlist::UI::View::Create(*this, playlist, Options);
      widgetsContainer->addTab(plView, playlist->GetName());
      this->connect(plView, SIGNAL(OnItemActivated(const Playlist::Item::Data&)),
        SLOT(PlaylistItemActivated(const Playlist::Item::Data&)));
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

    void AddItemsToVisiblePlaylist(const QStringList& items)
    {
      Playlist::UI::View& pl = GetVisiblePlaylist();
      pl.AddItems(items);
    }

    void SwitchToLastPlaylist()
    {
      Log::Debug(THIS_MODULE, "Move to another playlist");
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
        Log::Debug(THIS_MODULE, "Switching to playlist idx=%1% val=%2%", index, ActivePlaylistView);
      }
    }
  private:
    const Parameters::Container::Ptr Options;
    const Playlist::Container::Ptr Container;
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
      return new ContainerViewImpl(parent, parameters);
    }
  }
}
