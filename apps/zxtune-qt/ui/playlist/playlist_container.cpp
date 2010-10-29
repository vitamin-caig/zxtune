/*
Abstract:
  Playlist container implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "playlist_container.h"
#include "playlist_container_ui.h"
#include "playlist_container_moc.h"
#include "playlist_view.h"
#include "playlist_scanner.h"
#include "playlist.h"
#include "ui/utils.h"
//std includes
#include <cassert>
//qt includes
#include <QtGui/QMainWindow>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>

namespace
{
  const std::string THIS_MODULE("UI::Playlist");

  class PlaylistContainerImpl : public PlaylistContainer
  {
  public:
    explicit PlaylistContainerImpl(QObject* parent)
      : Provider(PlayitemsProvider::Create())
    {
      //setup self
      setParent(parent);
    }

    virtual PlaylistSupport* CreatePlaylist(const QString& name)
    {
      PlaylistSupport* const playlist = PlaylistSupport::Create(this, name, Provider);
      return playlist;
    }
  private:
    const PlayitemsProvider::Ptr Provider;
  };

  class PlaylistContainerViewImpl : public PlaylistContainerView
                                  , public Ui::PlaylistContainerView
  {
  public:
    explicit PlaylistContainerViewImpl(QMainWindow* parent)
      : Provider(PlayitemsProvider::Create())
      , Container(PlaylistContainer::Create(this))
      , ActivePlaylistView(0)
    {
      //setup self
      setParent(parent);
      setupUi(this);

      //create and fill menu
      QMenuBar* const menuBar = parent->menuBar();
      QMenu* const menu = menuBar->addMenu(QString::fromUtf8("Playlist"));
      menu->addAction(actionAddFiles);
      //menu->addAction(actionLoad);
      //menu->addAction(actionSave);
      menu->addSeparator();
      menu->addAction(actionClear);
      menu->addSeparator();
      menu->addAction(actionLoop);
      menu->addAction(actionRandom);
    }

    virtual void CreatePlaylist(const QStringList& items)
    {
      PlaylistSupport* const pl = Container->CreatePlaylist(tr("Default"));
      RegisterPlaylist(*pl);
      PlaylistScanner& scanner = pl->GetScanner();
      scanner.AddItems(items);
    }

    virtual void Play()
    {
      UpdateState(PLAYING);
    }

    virtual void Pause()
    {
      UpdateState(PAUSED);
    }

    virtual void Stop()
    {
      UpdateState(STOPPED);
    }
  private:
    void RegisterPlaylist(PlaylistSupport& playlist)
    {
      PlaylistWidget* const plView = PlaylistWidget::Create(this, playlist);
      widgetsContainer->addTab(plView, playlist.objectName());
      PlayitemIterator& iter = playlist.GetIterator();
      this->connect(&iter, SIGNAL(OnItem(const Playitem&)), SIGNAL(OnItemActivated(const Playitem&)));
      if (!ActivePlaylistView)
      {
        ActivePlaylistView = plView;
      }
    }

    void UpdateState(PlayitemState state)
    {
      if (ActivePlaylistView)
      {
        const PlaylistSupport& playlist = ActivePlaylistView->GetPlaylist();
        PlayitemIterator& iter = playlist.GetIterator();
        iter.SetState(state);
        ActivePlaylistView->Update();
      }
    }
  private:
    PlayitemsProvider::Ptr Provider;
    PlaylistContainer* const Container;
    //state context
    PlaylistWidget* ActivePlaylistView;
  };
}

PlaylistContainer* PlaylistContainer::Create(QObject* parent)
{
  return new PlaylistContainerImpl(parent);
}

PlaylistContainerView* PlaylistContainerView::Create(QMainWindow* parent)
{
  REGISTER_METATYPE(Playitem::Ptr);
  assert(parent);
  return new PlaylistContainerViewImpl(parent);
}
