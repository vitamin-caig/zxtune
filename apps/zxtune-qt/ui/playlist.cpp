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
#include "process_thread.h"
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
  //TODO: to utilities
  inline QString ToQString(const String& str)
  {
    return QString(str.c_str());
  }

  inline String FromQString(const QString& str)
  {
    String tmp(str.size(), '\0');
    std::transform(str.begin(), str.end(), tmp.begin(), boost::mem_fn(&QChar::toAscii));
    return tmp;
  }

  class PlaylistImpl : virtual public Playlist
                     , private Ui::Playlist
  {
  public:
    PlaylistImpl(QWidget* parent)
      : Thread(ProcessThread::Create(this))
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
    }

    virtual ~PlaylistImpl()
    {
      Thread->Cancel();
    }

    virtual void AddItemByPath(const String& itemPath)
    {
      Thread->AddItemPath(itemPath);
    }

    virtual void AddItem(const ModuleItem& item)
    {
      ZXTune::Module::Information info;
      item.Module->GetModuleInformation(info);
      String title;
      Parameters::FindByName(info.Properties, ZXTune::Module::ATTR_TITLE, title) ||
      Parameters::FindByName(info.Properties, ZXTune::Module::ATTR_PATH, title);
      new QListWidgetItem(ToQString(title), playList);
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
  };
}

Playlist* Playlist::Create(QWidget* parent)
{
  assert(parent);
  return new PlaylistImpl(parent);
}
