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
#include <apps/base/moduleitem.h>
//common includes
#include <logging.h>
#include <parameters.h>
//library includes
#include <core/module_attrs.h>
//qt includes
#include <QtCore/QUrl>
#include <QtCore/QMutex>
#include <QtCore/QThread>
#include <QtGui/QDragEnterEvent>
//std includes
#include <cassert>
//boost includes
#include <boost/bind.hpp>

namespace
{
  class PlaylistImpl : public Playlist
                     , private Ui::Playlist
  {
    typedef std::map<int_t, ModuleItem> ItemsMap;
  public:
    explicit PlaylistImpl(QWidget* parent)
      : Thread(ProcessThread::Create(this))
      , CurrentItem(-1, 0)
    {
      //setup self
      setParent(parent);
      setupUi(this);
      setAcceptDrops(true);
      //setup thread-related
      scanStatus->hide();
      scanStatus->connect(Thread, SIGNAL(OnScanStart()), SLOT(show()));
      this->connect(Thread, SIGNAL(OnProgress(const Log::MessageData&)), SLOT(ShowProgress(const Log::MessageData&)));
      this->connect(Thread, SIGNAL(OnGetItem(const ModuleItem&)), SLOT(AddItem(const ModuleItem&)));
      scanStatus->connect(Thread, SIGNAL(OnScanStop()), SLOT(hide()));
      Thread->connect(scanCancel, SIGNAL(clicked()), SLOT(Cancel()));
      this->connect(playList, SIGNAL(itemActivated(QListWidgetItem*)), SLOT(SelectItem(QListWidgetItem*)));
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
      if (Items.empty())
      {
        return;
      }
      const int_t maxIdx = Items.rbegin()->first;
      for (int_t curIdx = CurrentItem.first + 1; curIdx <= maxIdx; ++curIdx)
      {
        const ItemsMap::iterator iter = Items.find(curIdx);
        if (iter != Items.end())
        {
          playList->setCurrentRow(curIdx);
          StopItem();
          return SelectItem(playList->currentItem());
        }
      }
    }

    virtual void PrevItem()
    {
      if (Items.empty())
      {
        return;
      }
      const int_t minIdx = Items.begin()->first;
      for (int_t curIdx = CurrentItem.first - 1; curIdx >= minIdx; --curIdx)
      {
        const ItemsMap::iterator iter = Items.find(curIdx);
        if (iter != Items.end())
        {
          playList->setCurrentRow(curIdx);
          StopItem();
          return SelectItem(playList->currentItem());
        }
      }
    }

    virtual void PlayItem()
    {
      if (QListWidgetItem* item = CurrentItem.second)
      {
        QFont font = item->font();
        font.setBold(true);
        font.setItalic(false);
        item->setFont(font);
      }
    }
    
    virtual void PauseItem()
    {
      if (QListWidgetItem* item = CurrentItem.second)
      {
        QFont font = item->font();
        font.setBold(false);
        font.setItalic(true);
        item->setFont(font);
      }
    }
    
    virtual void StopItem()
    {
      if (QListWidgetItem* item = CurrentItem.second)
      {
        QFont font = item->font();
        font.setBold(false);
        font.setItalic(false);
        item->setFont(font);
      }
    }

    virtual void AddItem(const ModuleItem& item)
    {
      const Char RESOURCE_TYPE_PREFIX[] = {':','/','t','y','p','e','s','/',0};
      ZXTune::Module::Information info;
      item.Module->GetModuleInformation(info);
      String title;
      Parameters::FindByName(info.Properties, ZXTune::Module::ATTR_TITLE, title) ||
      Parameters::FindByName(info.Properties, ZXTune::Module::ATTR_FULLPATH, title);
      QListWidgetItem* const listItem = new QListWidgetItem(ToQString(title), playList);
      const int_t curIdx = Items.size();
      Items.insert(ItemsMap::value_type(curIdx, item));
      listItem->setData(Qt::UserRole, curIdx);
      if (const String* type = Parameters::FindByName<String>(info.Properties, ZXTune::Module::ATTR_TYPE))
      {
        String typeStr(RESOURCE_TYPE_PREFIX);
        typeStr += *type;
        listItem->setIcon(QIcon(ToQString(typeStr)));
      }
    }

    virtual void SelectItem(QListWidgetItem* listItem)
    {
      assert(listItem);
      const QVariant& data = listItem->data(Qt::UserRole);
      const int_t curIdx = data.toInt();
      const ItemsMap::iterator iter = Items.find(curIdx);
      if (iter != Items.end())
      {
        OnItemSelected(iter->second);
        CurrentItem = std::make_pair(curIdx, listItem);
      }
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
    ProcessThread* const Thread;
    ItemsMap Items;
    std::pair<int_t, QListWidgetItem*> CurrentItem;
  };
}

Playlist* Playlist::Create(QWidget* parent)
{
  assert(parent);
  return new PlaylistImpl(parent);
}
