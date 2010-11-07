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
#include "playlist/supp/playlist.h"
#include "playlist/supp/container.h"
#include "playlist/supp/model.h"
#include "playlist/supp/scanner.h"
#include "ui/utils.h"
//std includes
#include <cassert>
//qt includes
#include <QtCore/QUrl>
#include <QtGui/QFileDialog>
#include <QtGui/QMenu>

namespace
{
  class PlaylistContainerViewImpl : public PlaylistContainerView
                                  , public Ui::PlaylistContainerView
  {
  public:
    explicit PlaylistContainerViewImpl(QWidget* parent)
      //TODO: from global parameters
      : Container(PlaylistContainer::Create(this, Parameters::Container::Create(), Parameters::Container::Create()))
      , ActionsMenu(new QMenu(tr("Playlist"), this))
      , AddFileDirectory(QDir::currentPath())
      , ActivePlaylistView(0)
    {
      //setup self
      setParent(parent);
      setupUi(this);
      SetupMenu();

      //connect actions
      this->connect(actionAddFiles, SIGNAL(triggered()), SLOT(AddFiles()));
      this->connect(actionAddFolders, SIGNAL(triggered()), SLOT(AddFolders()));
      this->connect(actionClear, SIGNAL(triggered()), SLOT(Clear()));
    }

    virtual void CreatePlaylist(const QStringList& items)
    {
      const PlaylistSupport& playlist = CreateAnonymousPlaylist();
      PlaylistScanner& scanner = playlist.GetScanner();
      const bool deepScan = actionDeepScan->isChecked();
      scanner.AddItems(items, deepScan);
    }

    virtual QMenu* GetActionsMenu() const
    {
      return ActionsMenu;
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
      ProcessDialog(QFileDialog::ExistingFiles);
    }

    virtual void AddFolders()
    {
      ProcessDialog(QFileDialog::Directory);
    }
  private:
    void SetupMenu()
    {
      ActionsMenu->addAction(actionAddFiles);
      ActionsMenu->addAction(actionAddFolders);
      ActionsMenu->addAction(actionDeepScan);
      //ActionsMenu->addAction(actionLoad);
      //ActionsMenu->addAction(actionSave);
      ActionsMenu->addSeparator();
      ActionsMenu->addAction(actionClear);
      //ActionsMenu->addSeparator();
      //ActionsMenu->addAction(actionLoop);
      //ActionsMenu->addAction(actionRandom);
    }

    void ProcessDialog(QFileDialog::FileMode mode)
    {
      QFileDialog dialog(this);
      dialog.setAcceptMode(QFileDialog::AcceptOpen);
      dialog.setFileMode(mode);
      dialog.setViewMode(QFileDialog::Detail);
      dialog.setOption(QFileDialog::DontUseNativeDialog, true);
      dialog.setOption(QFileDialog::ReadOnly, true);
      dialog.setOption(QFileDialog::HideNameFilterDetails, true);
      dialog.setOption(QFileDialog::ShowDirsOnly, mode == QFileDialog::Directory);
      dialog.setDirectory(AddFileDirectory);
      if (QDialog::Accepted == dialog.exec())
      {
        AddFileDirectory = dialog.directory().absolutePath();
        const QStringList& files = dialog.selectedFiles();
        const PlaylistSupport& playlist = GetCurrentPlaylist();
        PlaylistScanner& scanner = playlist.GetScanner();
        const bool deepScan = actionDeepScan->isChecked();
        scanner.AddItems(files, deepScan);
      }
    }

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
    PlaylistContainer* const Container;
    QMenu* const ActionsMenu;
    //state context
    QString AddFileDirectory;
    PlaylistView* ActivePlaylistView;
  };
}

PlaylistContainerView* PlaylistContainerView::Create(QWidget* parent)
{
  REGISTER_METATYPE(Playitem::Ptr);
  assert(parent);
  return new PlaylistContainerViewImpl(parent);
}
