/*
Abstract:
  Playlist creating implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "playlist.h"
#include "playlist_ui.h"
#include "playlist_moc.h"
#include "playlist_model.h"
#include "playlist_scanner.h"
#include "playlist_scanner_view.h"
#include "playlist_view.h"
#include "ui/format.h"
#include "ui/utils.h"
//common includes
#include <logging.h>
#include <parameters.h>
//library includes
#include <core/module_attrs.h>
//qt includes
#include <QtGui/QFileDialog>
#include <QtGui/QMainWindow>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
//std includes
#include <cassert>
//boost includes
#include <boost/bind.hpp>
//text includes
#include "text/text.h"

namespace
{
  const std::string THIS_MODULE("UI::Playlist");

  class PlayitemStateCallbackImpl : public PlayitemStateCallback
  {
    enum PlayitemMode
    {
      STOPPED,
      PLAYING,
      PAUSED
    };
  public:
    PlayitemStateCallbackImpl()
      : OperationalItem(0)
      , OperationalMode(STOPPED)
    {
    }

    virtual bool IsPlaying(const QModelIndex& index) const
    {
      assert(index.isValid());
      const Playitem* const indexItem = static_cast<const Playitem*>(index.internalPointer());
      return OperationalItem &&
             indexItem == OperationalItem &&
             OperationalMode == PLAYING;
    }

    virtual bool IsPaused(const QModelIndex& index) const
    {
      assert(index.isValid());
      const Playitem* const indexItem = static_cast<const Playitem*>(index.internalPointer());
      return OperationalItem &&
             indexItem == OperationalItem &&
             OperationalMode == PAUSED;
    }

    void SetItem(const QModelIndex& index)
    {
      OperationalMode = STOPPED;
      assert(index.isValid());
      const Playitem* const indexItem = static_cast<const Playitem*>(index.internalPointer());
      OperationalItem = indexItem;
    }

    void ResetItem(const QModelIndex& index)
    {
      assert(index.isValid());
      const Playitem* const indexItem = static_cast<const Playitem*>(index.internalPointer());
      if (OperationalItem == indexItem)
      {
        OperationalItem = 0;
        OperationalMode = STOPPED;
      }
    }

    void PlayItem()
    {
      //assert(OperationalItem);
      OperationalMode = PLAYING;
    }

    void StopItem()
    {
      //assert(OperationalItem);
      OperationalMode = STOPPED;
    }

    void PauseItem()
    {
      //assert(OperationalItem);
      OperationalMode = PAUSED;
    }
  private:
    const Playitem* OperationalItem;
    PlayitemMode OperationalMode;
  };

  class PlaylistImpl : public Playlist
                     , public Ui::Playlist
  {
  public:
    explicit PlaylistImpl(QMainWindow* parent)
      : Provider(PlayitemsProvider::Create())
      , Scanner(PlaylistScanner::Create(this, Provider))
      , ScannerView(PlaylistScannerView::Create(this, Scanner))
      , Randomized(), Looped()
      , Model(PlaylistModel::Create(this))
      , View(PlaylistView::Create(this, State, Model, Scanner))
    {
      //setup self
      setParent(parent);
      setupUi(this);
      setAcceptDrops(true);
      verticalLayout->addWidget(View);
      verticalLayout->addWidget(ScannerView);
      //setup connections
      this->connect(View, SIGNAL(OnItemSet(const Playitem&)), SIGNAL(OnItemSet(const Playitem&)));
      this->connect(actionAddFiles, SIGNAL(triggered()), SLOT(AddFiles()));
      this->connect(actionClear, SIGNAL(triggered()), SLOT(Clear()));
      this->connect(actionRandom, SIGNAL(triggered(bool)), SLOT(Random(bool)));
      this->connect(actionLoop, SIGNAL(triggered(bool)), SLOT(Loop(bool)));
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

    virtual ~PlaylistImpl()
    {
      Scanner->Cancel();
      Scanner->wait();
    }

    virtual void AddItems(const QStringList& items)
    {
      View->AddItems(items);
    }

    virtual void NextItem()
    {
      //TODO
    }

    virtual void PrevItem()
    {
      //TODO
    }

    virtual void PlayItem()
    {
      State.PlayItem();
      View->Update();
    }

    virtual void PauseItem()
    {
      State.PauseItem();
      View->Update();
    }

    virtual void StopItem()
    {
      State.StopItem();
      View->Update();
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
        View->AddItems(files);
      }
    }

    virtual void Clear()
    {
      Model->Clear();
      Provider->ResetCache();
    }

    virtual void Random(bool isRandom)
    {
      Randomized = isRandom;
    }

    virtual void Loop(bool isLooped)
    {
      Looped = isLooped;
    }

  private:
    PlayitemsProvider::Ptr Provider;
    PlaylistScanner* const Scanner;
    PlaylistScannerView* const ScannerView;
    bool Randomized;
    bool Looped;
    PlayitemStateCallbackImpl State;
    PlaylistModel* const Model;
    PlaylistView* const View;
    //gui-related
    QString AddFileDirectory;
  };
}

Playlist* Playlist::Create(QMainWindow* parent)
{
  REGISTER_METATYPE(Playitem::Ptr);
  assert(parent);
  return new PlaylistImpl(parent);
}
