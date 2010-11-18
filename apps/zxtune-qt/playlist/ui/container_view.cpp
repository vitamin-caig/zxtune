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

  QString ExtractPlaylistName(const PlaylistIOContainer& container, const QString& defVal)
  {
    const Parameters::Accessor::Ptr params = container.GetProperties();
    Parameters::StringType val;
    if (params->FindStringValue(Playlist::ATTRIBUTE_NAME, val))
    {
      return ToQString(val);
    }
    return defVal;
  }

  class FileDialogWrapper
  {
  public:
    explicit FileDialogWrapper(QWidget& parent)
      : Dialog(&parent, QString(), QDir::currentPath())
    {
      Dialog.setViewMode(QFileDialog::Detail);
      Dialog.setOption(QFileDialog::DontUseNativeDialog, true);
      Dialog.setOption(QFileDialog::HideNameFilterDetails, true);
    }

    bool OpenSingleFile(const QString& title, const QString& filters, QString& file)
    {
      Dialog.setWindowTitle(title);
      Dialog.setNameFilter(filters);
      Dialog.setFileMode(QFileDialog::ExistingFile);
      Dialog.setOption(QFileDialog::ShowDirsOnly, false);
      SetROMode();

      if (ProcessDialog())
      {
        file = Dialog.selectedFiles().front();
        return true;
      }
      return false;
    }

    bool OpenMultipleFiles(const QString& title, const QString& filters, QStringList& files)
    {
      Dialog.setWindowTitle(title);
      Dialog.setNameFilter(filters);
      Dialog.setFileMode(QFileDialog::ExistingFiles);
      Dialog.setOption(QFileDialog::ShowDirsOnly, false);
      SetROMode();

      if (ProcessDialog())
      {
        files = Dialog.selectedFiles();
        return true;
      }
      return false;
    }

    bool OpenMultipleFolders(const QString& title, QStringList& folders)
    {
      Dialog.setWindowTitle(title);
      Dialog.setFileMode(QFileDialog::Directory);
      Dialog.setOption(QFileDialog::ShowDirsOnly, true);
      SetROMode();

      if (ProcessDialog())
      {
        folders = Dialog.selectedFiles();
        return true;
      }
      return false;
    }

    bool SaveFile(const QString& title, const QString& suffix, QString& filename)
    {
      Dialog.setWindowTitle(title);
      Dialog.setDefaultSuffix(suffix);
      Dialog.setNameFilter(QString::fromUtf8("*.") + suffix);
      Dialog.selectFile(filename);
      Dialog.setFileMode(QFileDialog::AnyFile);
      Dialog.setOption(QFileDialog::ShowDirsOnly, false);
      SetRWMode();
      if (ProcessDialog())
      {
        filename = Dialog.selectedFiles().front();
        return true;
      }
      return false;
    }
  private:
    bool ProcessDialog()
    {
      return QDialog::Accepted == Dialog.exec();
    }

    void SetROMode()
    {
      Dialog.setOption(QFileDialog::ReadOnly, true);
      Dialog.setAcceptMode(QFileDialog::AcceptOpen);
    }

    void SetRWMode()
    {
      Dialog.setOption(QFileDialog::ReadOnly, false);
      Dialog.setAcceptMode(QFileDialog::AcceptSave);
    }
  private:
    QFileDialog Dialog;
  };

  class PlaylistContainerViewImpl : public PlaylistContainerView
                                  , public Ui::PlaylistContainerView
  {
  public:
    explicit PlaylistContainerViewImpl(QWidget& parent)
      : ::PlaylistContainerView(parent)
      //TODO: from global parameters
      , Container(PlaylistContainer::Create(*this, Parameters::Container::Create(), Parameters::Container::Create()))
      , ActionsMenu(new QMenu(tr("Playlist"), this))
      , FileDialog(*this)
      , ActivePlaylistView(0)
    {
      //setup self
      setupUi(this);
      SetupMenu();

      //connect actions
      this->connect(actionAddFiles, SIGNAL(triggered()), SLOT(AddFiles()));
      this->connect(actionAddFolders, SIGNAL(triggered()), SLOT(AddFolders()));
      //playlist actions
      this->connect(actionCreatePlaylist, SIGNAL(triggered()), SLOT(CreatePlaylist()));
      this->connect(actionLoadPlaylist, SIGNAL(triggered()), SLOT(LoadPlaylist()));
      this->connect(actionSavePlaylist, SIGNAL(triggered()), SLOT(SavePlaylist()));
      this->connect(actionClosePlaylist, SIGNAL(triggered()), SLOT(CloseCurrentPlaylist()));
      this->connect(actionClearPlaylist, SIGNAL(triggered()), SLOT(Clear()));

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
      QStringList files;
      if (FileDialog.OpenMultipleFiles(actionAddFiles->text(), 
        tr("All files (*.*)"), files))
      {
        const PlaylistSupport& playlist = GetCurrentPlaylist();
        PlaylistScanner& scanner = playlist.GetScanner();
        const bool deepScan = actionDeepScan->isChecked();
        scanner.AddItems(files, deepScan);
      }
    }

    virtual void AddFolders()
    {
      QStringList folders;
      if (FileDialog.OpenMultipleFolders(actionAddFolders->text(), folders))
      {
        const PlaylistSupport& playlist = GetCurrentPlaylist();
        PlaylistScanner& scanner = playlist.GetScanner();
        const bool deepScan = actionDeepScan->isChecked();
        scanner.AddItems(folders, deepScan);
      }
    }

    virtual void CreatePlaylist()
    {
      CreateAnonymousPlaylist();
    }

    virtual void LoadPlaylist()
    {
      QString file;
      if (FileDialog.OpenSingleFile(actionLoadPlaylist->text(),
         tr("Playlist files (*.xspf *.ayl)"), file))
      {
        if (PlaylistSupport* const pl = Container->OpenPlaylist(file))
        {
          RegisterPlaylist(*pl);
        }
      }
    }

    virtual void SavePlaylist()
    {
      PlaylistView* const view = static_cast<PlaylistView*>(widgetsContainer->currentWidget());
      const PlaylistSupport& playlist = view->GetPlaylist();
      const PlaylistIOContainer::Ptr container = playlist.GetContainer();
      QString filename = ExtractPlaylistName(*container, playlist.GetName());
      if (FileDialog.SaveFile(actionSavePlaylist->text(),
        QString::fromUtf8("xspf"), filename))
      {
        if (!SaveXSPFPlaylist(container, filename))
        {
          assert(!"Failed to save");
        }
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
      ActionsMenu->addAction(actionCreatePlaylist);
      ActionsMenu->addAction(actionLoadPlaylist);
      ActionsMenu->addAction(actionSavePlaylist);
      ActionsMenu->addAction(actionClosePlaylist);
      ActionsMenu->addSeparator();
      ActionsMenu->addAction(actionClearPlaylist);
      //ActionsMenu->addSeparator();
      //ActionsMenu->addAction(actionLoop);
      //ActionsMenu->addAction(actionRandom);
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
      widgetsContainer->addTab(plView, playlist.GetName());
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
    FileDialogWrapper FileDialog;
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
