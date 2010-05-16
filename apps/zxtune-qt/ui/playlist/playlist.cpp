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
#include "../utils.h"
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

//outside the namespace
Q_DECLARE_METATYPE(Playitem::Ptr);

namespace
{
  enum ChooseMode
  {
    ITEM_SET,
    ITEM_SELECT
  };
  
  inline String GenerateItemTitle(const ZXTune::Module::Information& info)
  {
    String title;
    Parameters::FindByName(info.Properties, ZXTune::Module::ATTR_TITLE, title) ||
    Parameters::FindByName(info.Properties, ZXTune::Module::ATTR_FULLPATH, title);
    return title;
  }

  class PlaylistImpl : public Playlist
                     , private Ui::Playlist
  {
  public:
    explicit PlaylistImpl(QMainWindow* parent)
      : Thread(ProcessThread::Create(this))
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
      this->connect(Thread, SIGNAL(OnGetItem(const Playitem::Ptr&)), SLOT(AddItem(const Playitem::Ptr&)));
      scanStatus->connect(Thread, SIGNAL(OnScanStop()), SLOT(hide()));
      Thread->connect(scanCancel, SIGNAL(clicked()), SLOT(Cancel()));
      this->connect(playList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), SLOT(SetItem(QListWidgetItem*)));
      this->connect(playList, SIGNAL(itemActivated(QListWidgetItem*)), SLOT(SelectItem(QListWidgetItem*)));
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
      if (!rowsCount || !ActivatedItem)
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
      if (!rowsCount || !ActivatedItem)
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
      if (ActivatedItem)
      {
        QFont font = ActivatedItem->font();
        font.setBold(true);
        font.setItalic(false);
        ActivatedItem->setFont(font);
      }
    }
    
    virtual void PauseItem()
    {
      if (ActivatedItem)
      {
        QFont font = ActivatedItem->font();
        font.setItalic(true);
        ActivatedItem->setFont(font);
      }
    }
    
    virtual void StopItem()
    {
      if (ActivatedItem)
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
      if (QDialog::Accepted == dialog.exec())
      {
        const QStringList& files = dialog.selectedFiles();
        std::for_each(files.begin(), files.end(),
          boost::bind(&Playlist::AddItemByPath, this,
            boost::bind(&FromQString, _1)));
      }
    }
    
    virtual void Clear()
    {
      playList->clear();
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

    //private slots
    virtual void AddItem(const Playitem::Ptr& item)
    {
      const Char RESOURCE_TYPE_PREFIX[] = {':','/','t','y','p','e','s','/',0};
      const ZXTune::Module::Information& info = item->GetModuleInfo();
      const String& title = GenerateItemTitle(info);
      QListWidgetItem* const listItem = new QListWidgetItem(ToQString(title), playList);
      listItem->setData(Qt::UserRole, QVariant::fromValue(item));
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
          boost::bind(&Playlist::AddItemByPath, this,
            boost::bind(&FromQString, boost::bind(&QUrl::toLocalFile, _1))));
      }
    }
  private:
    void ChooseItem(QListWidgetItem* listItem, ChooseMode mode)
    {
      assert(listItem);
      const QVariant& data = listItem->data(Qt::UserRole);
      const Playitem::Ptr& item = data.value<Playitem::Ptr>();
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
  private:
    ProcessThread* const Thread;
    bool Randomized;
    bool Looped;
    QListWidgetItem* ActivatedItem;
    QListWidgetItem* SelectedItem;
  };
}

Playlist* Playlist::Create(QMainWindow* parent)
{
  assert(parent);
  return new PlaylistImpl(parent);
}
