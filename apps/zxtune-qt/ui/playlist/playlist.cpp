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
typedef std::list<Playitem::Ptr> PlayitemsList;
Q_DECLARE_METATYPE(PlayitemsList::iterator);

namespace
{
  enum ChooseMode
  {
    ITEM_SET,
    ITEM_SELECT
  };
  
  QListWidgetItem ITEM_STUB;

  class PlaylistImpl : public Playlist
                     , private Ui::Playlist
  {
  public:
    explicit PlaylistImpl(QMainWindow* parent)
      : Provider(PlayitemsProvider::Create())
      , Thread(ProcessThread::Create(this, Provider))
      , Randomized(), Looped()
      , ActivatedItem(), SelectedItem()
    {
      //setup self
      setParent(parent);
      setupUi(this);
      setAcceptDrops(true);
      //setup thread-related
      scanStatus->hide();
      scanStatus->connect(Thread, SIGNAL(OnScanStart()), SLOT(show()));
      this->connect(Thread, SIGNAL(OnProgress(const Log::MessageData&)), SLOT(ShowProgress(const Log::MessageData&)));
      this->connect(Thread, SIGNAL(OnGetItem(Playitem::Ptr)), SLOT(AddItem(Playitem::Ptr)));
      scanStatus->connect(Thread, SIGNAL(OnScanStop()), SLOT(hide()));
      Thread->connect(scanCancel, SIGNAL(clicked()), SLOT(Cancel()));
      this->connect(playList, SIGNAL(itemActivated(QListWidgetItem*)), SLOT(SetItem(QListWidgetItem*)));
      this->connect(playList, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)), SLOT(SelectItem(QListWidgetItem*)));
      this->connect(actionAddFiles, SIGNAL(triggered()), SLOT(AddFiles()));
      this->connect(actionClear, SIGNAL(triggered()), SLOT(Clear()));
      this->connect(actionSort, SIGNAL(triggered()), SLOT(Sort()));
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
      menu->addAction(actionSort);
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
      const int rowsCount = playList->count();
      if (!rowsCount || !ActivatedItem || ActivatedItem == &ITEM_STUB)
      {
        return;
      }
      const int currentRow = playList->row(ActivatedItem);
      const int nextRow = (Randomized ? rand() : (currentRow + 1)) % rowsCount;
      assert(currentRow >= 0);
      if (Looped || Randomized || currentRow < rowsCount - 1)
      {
        playList->setCurrentRow(nextRow, QItemSelectionModel::NoUpdate);
        return ChooseItem(playList->currentItem(), ITEM_SET);
      }
    }

    virtual void PrevItem()
    {
      const int rowsCount = playList->count();
      if (!rowsCount || !ActivatedItem || ActivatedItem == &ITEM_STUB)
      {
        return;
      }
      const int currentRow = playList->row(ActivatedItem);
      const int nextRow = (Randomized ? rand() : (currentRow + rowsCount - 1)) % rowsCount;
      assert(currentRow >= 0);
      if (Looped || Randomized || currentRow)
      {
        playList->setCurrentRow(nextRow, QItemSelectionModel::NoUpdate);
        return ChooseItem(playList->currentItem(), ITEM_SET);
      }
    }

    virtual void PlayItem()
    {
      if (!ActivatedItem)
      {
        ActivatedItem = SelectedItem;
      }
      if (ActivatedItem && ActivatedItem != &ITEM_STUB)
      {
        QFont font = ActivatedItem->font();
        font.setBold(true);
        font.setItalic(false);
        ActivatedItem->setFont(font);
      }
    }
    
    virtual void PauseItem()
    {
      if (ActivatedItem && ActivatedItem != &ITEM_STUB)
      {
        QFont font = ActivatedItem->font();
        font.setItalic(true);
        ActivatedItem->setFont(font);
      }
    }
    
    virtual void StopItem()
    {
      if (ActivatedItem && ActivatedItem != &ITEM_STUB)
      {
        QFont font = ActivatedItem->font();
        font.setBold(false);
        font.setItalic(false);
        ActivatedItem->setFont(font);
      }
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
      playList->clear();
      Items.clear();
      Provider->ResetCache();
      ActivatedItem = SelectedItem = 0;
    }
    
    virtual void Sort()
    {
      playList->sortItems();
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
    virtual void AddItem(Playitem::Ptr item)
    {
      const Char RESOURCE_TYPE_PREFIX[] = {':','/','t','y','p','e','s','/',0};
      const ZXTune::Module::Information& info = item->GetModuleInfo();

      const String& title = GetModuleTitle(Text::MODULE_PLAYLIST_FORMAT, info);
      QListWidgetItem* const listItem = new QListWidgetItem(ToQString(title), playList);
      const PlayitemsList::iterator iter = Items.insert(Items.end(), item);
      listItem->setData(Qt::UserRole, QVariant::fromValue(iter));
      if (const String* type = Parameters::FindByName<String>(info.Properties, ZXTune::Module::ATTR_TYPE))
      {
        String typeStr(RESOURCE_TYPE_PREFIX);
        typeStr += *type;
        listItem->setIcon(QIcon(ToQString(typeStr)));
      }
    }
    
    virtual void SelectItem(QListWidgetItem* listItem)
    {
      ChooseItem(listItem, ITEM_SELECT);
    }

    virtual void SetItem(QListWidgetItem* listItem)
    {
      ChooseItem(listItem, ITEM_SET);
    }

    virtual void ClearSelected()
    {
      const QList<QListWidgetItem*>& items = playList->selectedItems();
      std::for_each(items.begin(), items.end(), boost::bind(&PlaylistImpl::RemoveItem, this, _1));
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
    void ChooseItem(QListWidgetItem* listItem, ChooseMode mode)
    {
      if (!listItem)
      {
        ActivatedItem = SelectedItem = 0;
      }
      else
      {
        const QVariant& data = listItem->data(Qt::UserRole);
        const PlayitemsList::iterator iter = data.value<PlayitemsList::iterator>();
        if (ITEM_SET == mode)
        {
          StopItem();
          ActivatedItem = 0;
          SelectedItem = listItem;
          OnItemSet(**iter);
        }
        else if (ITEM_SELECT == mode)
        {
          SelectedItem = listItem;
          OnItemSelected(**iter);
        }
      }
    }

    void RemoveItem(QListWidgetItem* listItem)
    {
      const QVariant& data = listItem->data(Qt::UserRole);
      const PlayitemsList::iterator iter = data.value<PlayitemsList::iterator>();
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
      const int row = playList->row(listItem);
      std::auto_ptr<QListWidgetItem> removed(playList->takeItem(row));
      assert(removed.get() == listItem);
      Items.erase(iter);
    }
  private:
    PlayitemsProvider::Ptr Provider;
    ProcessThread* const Thread;
    bool Randomized;
    bool Looped;
    //TODO: thread-safe
    PlayitemsList Items;
    QListWidgetItem* ActivatedItem;
    QListWidgetItem* SelectedItem;
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
