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
#include "playlist_thread.h"
#include "playlist_view.h"
#include "ui/format.h"
#include "ui/utils.h"
//common includes
#include <logging.h>
#include <parameters.h>
//library includes
#include <core/module_attrs.h>
//qt includes
#include <QtCore/QUrl>
#include <QtGui/QDragEnterEvent>
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
      assert(OperationalItem);
      OperationalMode = PLAYING;
    }

    void StopItem()
    {
      assert(OperationalItem);
      OperationalMode = STOPPED;
    }

    void PauseItem()
    {
      assert(OperationalItem);
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
      , Thread(ProcessThread::Create(this, Provider))
      , Randomized(), Looped()
      , Model(PlaylistModel::Create(this))
      , View(PlaylistView::Create(State, this))
    {
      //setup self
      setParent(parent);
      setupUi(this);
      setAcceptDrops(true);
      verticalLayout->insertWidget(0, View);
      View->setModel(Model);
      //setup thread-related
      scanStatus->hide();
      scanStatus->connect(Thread, SIGNAL(OnScanStart()), SLOT(show()));
      this->connect(Thread, SIGNAL(OnProgress(unsigned, unsigned, unsigned)), SLOT(ShowProgress(unsigned)));
      this->connect(Thread, SIGNAL(OnProgressMessage(const QString&, const QString&)), SLOT(ShowProgressMessage(const QString&)));
      Model->connect(Thread, SIGNAL(OnGetItem(Playitem::Ptr)), SLOT(AddItem(Playitem::Ptr)));
      scanStatus->connect(Thread, SIGNAL(OnScanStop()), SLOT(hide()));
      Thread->connect(scanCancel, SIGNAL(clicked()), SLOT(Cancel()));
      this->connect(View, SIGNAL(activated(const QModelIndex&)), SLOT(ActivateItem(const QModelIndex&)));
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
      Thread->Cancel();
    }

    virtual void AddItems(const QStringList& items)
    {
      Thread->AddItems(items);
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
      View->viewport()->update();
    }

    virtual void PauseItem()
    {
      State.PauseItem();
      View->viewport()->update();
    }

    virtual void StopItem()
    {
      State.StopItem();
      View->viewport()->update();
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
        AddItems(files);
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

    //QWidget virtuals
    virtual void keyReleaseEvent(QKeyEvent* event)
    {
      const int curKey = event->key();
      if (curKey == Qt::Key_Delete || curKey == Qt::Key_Backspace)
      {
        ClearSelected();
      }
      else
      {
        QWidget::keyReleaseEvent(event);
      }
    }

    //private slots
    virtual void ActivateItem(const QModelIndex& index)
    {
      if (const Playitem::Ptr item = Model->GetItem(index.row()))
      {
        State.SetItem(index);
        OnItemSet(*item);
      }
    }

    virtual void ClearSelected()
    {
      const QItemSelectionModel* const selection = View->selectionModel();
      const QModelIndexList& items = selection->selectedRows();
      QSet<unsigned> indexes;
      std::for_each(items.begin(), items.end(),
        boost::bind(&QSet<unsigned>::insert, &indexes, 
          boost::bind(&QModelIndex::row, _1)));
      Model->RemoveItems(indexes);
      std::for_each(items.begin(), items.end(), boost::bind(&PlayitemStateCallbackImpl::ResetItem, &State, _1));
    }

    virtual void ShowProgress(unsigned progress)
    {
      scanProgress->setValue(progress);
    }

    virtual void ShowProgressMessage(const QString& message)
    {
      scanProgress->setToolTip(message);
    }

    //qwidget virtuals
    virtual void dragEnterEvent(QDragEnterEvent* event)
    {
      event->acceptProposedAction();
    }

    virtual void dropEvent(QDropEvent* event)
    {
      if (event->mimeData()->hasUrls())
      {
        const QList<QUrl>& urls = event->mimeData()->urls();
        QStringList files;
        std::for_each(urls.begin(), urls.end(),
          boost::bind(&QStringList::push_back, &files,
            boost::bind(&QUrl::toLocalFile, _1)));
        AddItems(files);
      }
    }
  private:
    PlayitemsProvider::Ptr Provider;
    ProcessThread* const Thread;
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
