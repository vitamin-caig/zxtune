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
#include "playlist_model.h"
#include "playlist_scanner.h"
#include "playlist.h"
#include "ui/utils.h"
//std includes
#include <cassert>
//qt includes
#include <QtGui/QFileDialog>
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
      //menu->addSeparator();
      //menu->addAction(actionLoop);
      //menu->addAction(actionRandom);

      //connect actions
      this->connect(actionAddFiles, SIGNAL(triggered()), SLOT(AddFiles()));
      this->connect(actionClear, SIGNAL(triggered()), SLOT(Clear()));
    }

    virtual void CreatePlaylist(const QStringList& items)
    {
      const PlaylistSupport& playlist = CreateAnonymousPlaylist();
      PlaylistScanner& scanner = playlist.GetScanner();
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

    virtual void Finish()
    {
      const PlaylistSupport& playlist = GetCurrentPlaylist();
      PlayitemIterator& iter = playlist.GetIterator();
      if (!iter.Next())
      {
        Stop();
      }
    }

    virtual void Next()
    {
      const PlaylistSupport& playlist = GetCurrentPlaylist();
      PlayitemIterator& iter = playlist.GetIterator();
      iter.Next();
    }

    virtual void Prev()
    {
      const PlaylistSupport& playlist = GetCurrentPlaylist();
      PlayitemIterator& iter = playlist.GetIterator();
      iter.Prev();
    }

    virtual void Clear()
    {
      const PlaylistSupport& playlist = GetCurrentPlaylist();
      PlaylistModel& model = playlist.GetModel();
      model.Clear();
      ActivePlaylistView->Update();
    }

    virtual void AddFiles()
    {
      QFileDialog dialog(this);
      dialog.setAcceptMode(QFileDialog::AcceptOpen);
      dialog.setFileMode(QFileDialog::ExistingFiles);
      dialog.setDirectory(AddFileDirectory);
      if (QDialog::Accepted == dialog.exec())
      {
        AddFileDirectory = dialog.directory().absolutePath();
        const QStringList& files = dialog.selectedFiles();
        const PlaylistSupport& playlist = GetCurrentPlaylist();
        PlaylistScanner& scanner = playlist.GetScanner();
        scanner.AddItems(files);
      }
    }
  private:
    PlaylistSupport& CreateAnonymousPlaylist()
    {
      PlaylistSupport* const pl = Container->CreatePlaylist(tr("Default"));
      RegisterPlaylist(*pl);
      return *pl;
    }

    void RegisterPlaylist(PlaylistSupport& playlist)
    {
      PlaylistView* const plView = PlaylistView::Create(this, playlist);
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
      const PlaylistSupport& playlist = GetCurrentPlaylist();
      PlayitemIterator& iter = playlist.GetIterator();
      iter.SetState(state);
      ActivePlaylistView->Update();
    }

    const PlaylistSupport& GetCurrentPlaylist()
    {
      if (!ActivePlaylistView)
      {
        CreateAnonymousPlaylist();
      }
      return ActivePlaylistView->GetPlaylist();
    }
  private:
    PlayitemsProvider::Ptr Provider;
    PlaylistContainer* const Container;
    //state context
    QString AddFileDirectory;
    PlaylistView* ActivePlaylistView;
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
