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

//outside the namespace
Q_DECLARE_METATYPE(Playitem::Ptr);

namespace
{
  enum ChooseMode
  {
    ITEM_SET,
    ITEM_SELECT
  };

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
      : OperationalMode(STOPPED)
    {
    }

    virtual bool IsPlaying(const QModelIndex& index) const
    {
      assert(index.isValid());
      return index == OperationalItem &&
             OperationalMode == PLAYING;
    }

    virtual bool IsPaused(const QModelIndex& index) const
    {
      assert(index.isValid());
      return index == OperationalItem &&
             OperationalMode == PAUSED;
    }

    void SetItem(const QModelIndex& index)
    {
      assert(index.isValid());
      OperationalItem = index;
      OperationalMode = STOPPED;
    }

    void ResetItem(const QModelIndex& index)
    {
      assert(index.isValid());
      if (OperationalItem == index)
      {
        OperationalItem == QModelIndex();
        OperationalMode = STOPPED;
      }
    }

    void PlayItem()
    {
      assert(OperationalItem.isValid());
      OperationalMode = PLAYING;
    }

    void StopItem()
    {
      assert(OperationalItem.isValid());
      OperationalMode = STOPPED;
    }

    void PauseItem()
    {
      assert(OperationalItem.isValid());
      OperationalMode = PAUSED;
    }

    const QModelIndex& GetItem() const
    {
      return OperationalItem;
    }
  private:
    QModelIndex OperationalItem;
    PlayitemMode OperationalMode;
  };

  //QListWidgetItem ITEM_STUB;

  class PlaylistImpl : public Playlist
                     , public Ui::Playlist
  {
  public:
    explicit PlaylistImpl(QMainWindow* parent)
      : Provider(PlayitemsProvider::Create())
      , Thread(ProcessThread::Create(this, Provider))
      , Randomized(), Looped()
      , Model(PlaylistModel::Create(State, this))
    {
      //setup self
      setParent(parent);
      setupUi(this);
      setAcceptDrops(true);
      playListContent->setModel(Model);
      {
        QHeaderView* header = playListContent->horizontalHeader();
        header->setDefaultAlignment(Qt::AlignLeft);
        header->setTextElideMode(Qt::ElideRight);
      }
      //setup thread-related
      scanStatus->hide();
      scanStatus->connect(Thread, SIGNAL(OnScanStart()), SLOT(show()));
      this->connect(Thread, SIGNAL(OnProgress(const Log::MessageData&)), SLOT(ShowProgress(const Log::MessageData&)));
      Model->connect(Thread, SIGNAL(OnGetItem(Playitem::Ptr)), SLOT(AddItem(Playitem::Ptr)));
      scanStatus->connect(Thread, SIGNAL(OnScanStop()), SLOT(hide()));
      Thread->connect(scanCancel, SIGNAL(clicked()), SLOT(Cancel()));
      this->connect(playListContent, SIGNAL(activated(const QModelIndex&)), SLOT(ActivateItem(const QModelIndex&)));
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

    virtual void AddItemByPath(const String& itemPath)
    {
      Thread->AddItemPath(itemPath);
    }

    virtual void NextItem()
    {
      /*
      const int rowsCount = playListContent->count();
      if (!rowsCount || !ActivatedItem || ActivatedItem == &ITEM_STUB)
      {
        return;
      }
      const int currentRow = playListContent->row(ActivatedItem);
      const int nextRow = (Randomized ? rand() : (currentRow + 1)) % rowsCount;
      assert(currentRow >= 0);
      if (Looped || Randomized || currentRow < rowsCount - 1)
      {
        playListContent->setCurrentRow(nextRow, QItemSelectionModel::NoUpdate);
        return ChooseItem(playListContent->currentItem(), ITEM_SET);
      }
      */
    }

    virtual void PrevItem()
    {
      /*
      const int rowsCount = playListContent->count();
      if (!rowsCount || !ActivatedItem || ActivatedItem == &ITEM_STUB)
      {
        return;
      }
      const int currentRow = playListContent->row(ActivatedItem);
      const int nextRow = (Randomized ? rand() : (currentRow + rowsCount - 1)) % rowsCount;
      assert(currentRow >= 0);
      if (Looped || Randomized || currentRow)
      {
        playListContent->setCurrentRow(nextRow, QItemSelectionModel::NoUpdate);
        return ChooseItem(playListContent->currentItem(), ITEM_SET);
      }
      */
    }

    virtual void PlayItem()
    {
      State.PlayItem();
      playListContent->update(State.GetItem());
    }

    virtual void PauseItem()
    {
      State.PauseItem();
      playListContent->update(State.GetItem());
    }

    virtual void StopItem()
    {
      State.StopItem();
      playListContent->update(State.GetItem());
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
        std::for_each(files.begin(), files.end(),
          boost::bind(&::Playlist::AddItemByPath, this,
            boost::bind(&FromQString, _1)));
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
        //ClearSelected();
      }
      else
      {
        QWidget::keyReleaseEvent(event);
      }
    }

    //private slots
    virtual void ActivateItem(const QModelIndex& index)
    {
      if (const Playitem::Ptr item = Model->GetItem(index))
      {
        State.SetItem(index);
        OnItemSet(*item);
      }
    }

    virtual void ClearSelected()
    {
      const QItemSelectionModel* const selection = playListContent->selectionModel();
      const QModelIndexList& items = selection->selectedRows();
      Model->RemoveItems(items);
      std::for_each(items.begin(), items.end(), boost::bind(&PlayitemStateCallbackImpl::ResetItem, &State, _1));
    }

    virtual void ShowProgress(const Log::MessageData& msg)
    {
      assert(0 == msg.Level);
      if (msg.Progress)
      {
        scanProgress->setValue(*msg.Progress);
      }
      if (msg.Text)
      {
        scanProgress->setToolTip(ToQString(*msg.Text));
      }
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
        std::for_each(urls.begin(), urls.end(),
          boost::bind(&::Playlist::AddItemByPath, this,
            boost::bind(&FromQString, boost::bind(&QUrl::toLocalFile, _1))));
      }
    }
  private:
    /*
    void ChooseItem(QListWidgetItem* listItem, ChooseMode mode)
    {
      if (!listItem)
      {
        ActivatedItem = SelectedItem = 0;
      }
      else
      {
        const QVariant& data = listItem->data(Qt::UserRole);
        const Playitem::Ptr item = data.value<Playitem::Ptr>();
        if (ITEM_SET == mode)
        {
          StopItem();
          ActivatedItem = 0;
          SelectedItem = listItem;
          OnItemSet(*item);
        }
        else if (ITEM_SELECT == mode)
        {
          SelectedItem = listItem;
          OnItemSelected(*item);
        }
      }
    }

    void RemoveItem(QListWidgetItem* listItem)
    {
      //check if selected affected
      if (SelectedItem == listItem)
      {
        SelectedItem = 0;
      }
      if (ActivatedItem == listItem)
      {
        ActivatedItem = &ITEM_STUB;
      }
      //remove
      const int row = playListContent->row(listItem);
      std::auto_ptr<QListWidgetItem> removed(playListContent->takeItem(row));
      assert(removed.get() == listItem);
    }
    */
  private:
    PlayitemsProvider::Ptr Provider;
    ProcessThread* const Thread;
    bool Randomized;
    bool Looped;
    //TODO: thread-safe
    PlayitemStateCallbackImpl State;
    PlaylistModel* const Model;
    //gui-related
    QString AddFileDirectory;
  };
}

Playlist* Playlist::Create(QMainWindow* parent)
{
  REGISTER_METATYPE(Log::MessageData);
  assert(parent);
  return new PlaylistImpl(parent);
}
