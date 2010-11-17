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
//common includes
#include <logging.h>
//std includes
#include <cassert>
//qt includes
#include <QtCore/QUrl>
#include <QtGui/QFileDialog>
#include <QtGui/QMenu>

namespace
{
  const std::string THIS_MODULE("Playlist::Container::View");

  class PlaylistContainerViewImpl : public PlaylistContainerView
                                  , public Ui::PlaylistContainerView
  {
  public:
    explicit PlaylistContainerViewImpl(QWidget& parent)
      : ::PlaylistContainerView(parent)
      //TODO: from global parameters
      , Container(PlaylistContainer::Create(*this, Parameters::Container::Create(), Parameters::Container::Create()))
      , ActionsMenu(new QMenu(tr("Playlist"), this))
      , AddFileDirectory(QDir::currentPath())
      , ActivePlaylistView(0)
    {
      //setup self
      setupUi(this);
      SetupMenu();

      //connect actions
      this->connect(actionAddFiles, SIGNAL(triggered()), SLOT(AddFiles()));
      this->connect(actionAddFolders, SIGNAL(triggered()), SLOT(AddFolders()));
      this->connect(actionLoad, SIGNAL(triggered()), SLOT(LoadPlaylist()));
      this->connect(actionClose, SIGNAL(triggered()), SLOT(CloseCurrentPlaylist()));

      this->connect(actionClear, SIGNAL(triggered()), SLOT(Clear()));

      this->connect(widgetsContainer, SIGNAL(tabCloseRequested(int)), SLOT(ClosePlaylist(int)));

      Log::Debug(THIS_MODULE, "Created at %1%", this);
    }

    virtual ~PlaylistContainerViewImpl()
    {
      Log::Debug(THIS_MODULE, "Destroyed at %1%", this);
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
      ProcessFilesDialog(QFileDialog::ExistingFiles);
    }

    virtual void AddFolders()
    {
      ProcessFilesDialog(QFileDialog::Directory);
    }

    virtual void LoadPlaylist()
    {
      QStringList files;
      if (!ProcessDialog(QFileDialog::ExistingFile, files))
      {
        return;
      }
      assert(files.size() == 1);
      if (PlaylistSupport* const pl = Container->OpenPlaylist(files.front()))
      {
        RegisterPlaylist(*pl);
      }
    }

    virtual void CloseCurrentPlaylist()
    {
      ClosePlaylist(widgetsContainer->currentIndex());
    }

    virtual void ClosePlaylist(int index)
    {
      PlaylistView* const view = static_cast<PlaylistView*>(widgetsContainer->widget(index));
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
  private:
    void PlaylistItemActivated(const class Playitem& item)
    {
      if (QObject* sender = this->sender())
      {
        assert(dynamic_cast<PlaylistView*>(sender));
        PlaylistView* const newView = static_cast<PlaylistView*>(sender);
        if (newView != ActivePlaylistView)
        {
          Log::Debug(THIS_MODULE, "Switched playlist %1% -> %2%", newView, ActivePlaylistView);
          UpdateState(STOPPED);
          ActivePlaylistView = newView;
        }
      }
      OnItemActivated(item);
    }
  private:
    void SetupMenu()
    {
      ActionsMenu->addAction(actionAddFiles);
      ActionsMenu->addAction(actionAddFolders);
      ActionsMenu->addAction(actionDeepScan);
      ActionsMenu->addSeparator();
      ActionsMenu->addAction(actionLoad);
      ActionsMenu->addAction(actionClose);
      //ActionsMenu->addAction(actionSave);
      ActionsMenu->addSeparator();
      ActionsMenu->addAction(actionClear);
      //ActionsMenu->addSeparator();
      //ActionsMenu->addAction(actionLoop);
      //ActionsMenu->addAction(actionRandom);
    }

    void ProcessFilesDialog(QFileDialog::FileMode mode)
    {
      QStringList files;
      if (ProcessDialog(mode, files))
      {
        const PlaylistSupport& playlist = GetCurrentPlaylist();
        PlaylistScanner& scanner = playlist.GetScanner();
        const bool deepScan = actionDeepScan->isChecked();
        scanner.AddItems(files, deepScan);
      }
    }

    bool ProcessDialog(QFileDialog::FileMode mode, QStringList& files)
    {
      QFileDialog dialog(this);
      dialog.setFileMode(mode);
      dialog.setAcceptMode(QFileDialog::AcceptOpen);
      dialog.setViewMode(QFileDialog::Detail);
      dialog.setOption(QFileDialog::DontUseNativeDialog, true);
      dialog.setOption(QFileDialog::ReadOnly, true);
      dialog.setOption(QFileDialog::HideNameFilterDetails, true);
      dialog.setOption(QFileDialog::ShowDirsOnly, mode == QFileDialog::Directory);
      dialog.setDirectory(AddFileDirectory);
      if (QDialog::Accepted == dialog.exec())
      {
        AddFileDirectory = dialog.directory().absolutePath();
        files = dialog.selectedFiles();
        return true;
      }
      return false;
    }

    PlaylistSupport& CreateAnonymousPlaylist()
    {
      Log::Debug(THIS_MODULE, "Create default playlist");
      PlaylistSupport* const pl = Container->CreatePlaylist(tr("Default"));
      RegisterPlaylist(*pl);
      return *pl;
    }

    void RegisterPlaylist(PlaylistSupport& playlist)
    {
      PlaylistView* const plView = PlaylistView::Create(*this, playlist);
      widgetsContainer->addTab(plView, playlist.objectName());
      this->connect(plView, SIGNAL(OnItemActivated(const Playitem&)),
        SLOT(PlaylistItemActivated(const Playitem&)));
      if (!ActivePlaylistView)
      {
        ActivePlaylistView = plView;
      }
      widgetsContainer->setCurrentWidget(plView);
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
        ActivePlaylistView = static_cast<PlaylistView*>(widget);
        Log::Debug(THIS_MODULE, "Switching to playlist idx=%1% val=%2%", index, ActivePlaylistView);
      }
    }
  private:
    PlaylistContainer* const Container;
    QMenu* const ActionsMenu;
    //state context
    QString AddFileDirectory;
    PlaylistView* ActivePlaylistView;
  };
}

PlaylistContainerView::PlaylistContainerView(QWidget& parent) : QWidget(&parent)
{
}

PlaylistContainerView* PlaylistContainerView::Create(QWidget& parent)
{
  REGISTER_METATYPE(Playitem::Ptr);
  return new PlaylistContainerViewImpl(parent);
}
