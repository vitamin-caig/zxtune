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
#include <QtCore/QMutex>
#include <QtCore/QThread>
//std includes
#include <cassert>

namespace
{
  //TODO: to utilities
  QString ToQString(const String& str)
  {
    return QString(str.c_str());
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
      //scanStatus->hide();
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
  private:
    ProcessThread* const Thread;
  };
}

Playlist* Playlist::Create(QWidget* parent)
{
  assert(parent);
  return new PlaylistImpl(parent);
}
