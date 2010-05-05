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
#include "utils.h"
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

namespace
{
  class PlaylistImpl : public Playlist
                     , private Ui::Playlist
  {
    typedef std::map<uint_t, ModuleItem> ItemsMap;
  public:
    explicit PlaylistImpl(QWidget* parent)
      : Thread(ProcessThread::Create(this))
      , CurrentItem(0)
    {
      setParent(parent);
      setupUi(this);
      setAcceptDrops(true);
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
      const uint_t maxIdx = Items.end()->first;
      for (uint_t curIdx = CurrentItem + 1; curIdx <= maxIdx; ++curIdx)
      {
        const ItemsMap::iterator iter = Items.find(curIdx);
        if (iter != Items.end())
        {
          //TODO:
          playList->setCurrentRow(CurrentItem = curIdx);
          return;
        }
      }
    }

    virtual void AddItem(const ModuleItem& item)
    {
      ZXTune::Module::Information info;
      item.Module->GetModuleInformation(info);
      String title;
      Parameters::FindByName(info.Properties, ZXTune::Module::ATTR_TITLE, title) ||
      Parameters::FindByName(info.Properties, ZXTune::Module::ATTR_FULLPATH, title);
      QListWidgetItem* const listItem = new QListWidgetItem(ToQString(title), playList);
      const uint_t curIdx = Items.size();
      Items.insert(ItemsMap::value_type(curIdx, item));
      listItem->setData(Qt::UserRole, curIdx);
    }

    virtual void SelectItem(QListWidgetItem* listItem)
    {
      const QVariant& data = listItem->data(Qt::UserRole);
      const uint_t curIdx = data.toUInt();
      const ItemsMap::iterator iter = Items.find(curIdx);
      if (iter != Items.end())
      {
        OnItemSelected(iter->second);
        CurrentItem = curIdx;
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
    uint_t CurrentItem;
  };
}

Playlist* Playlist::Create(QWidget* parent)
{
  assert(parent);
  return new PlaylistImpl(parent);
}
