/*
Abstract:
  Playitems process thread implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "playlist_thread.h"
#include "playlist_thread_moc.h"
#include "ui/utils.h"
//common includes
#include <error.h>
#include <logging.h>
//qt includes
#include <QtCore/QMutex>
#include <QtCore/QStringList>
//std includes
#include <ctime>
//boost includes
#include <boost/bind.hpp>

namespace
{
  class ProcessThreadImpl : public ProcessThread
                          , private PlayitemDetectParameters
  {
  public:
    ProcessThreadImpl(QObject* owner, PlayitemsProvider::Ptr provider)
      : Provider(provider)
      , Canceled(false)
    {
      setParent(owner);
    }

    virtual void AddItems(const QStringList& items)
    {
      QMutexLocker lock(&QueueLock);
      //TODO: process directories separately
      Queue.append(items);
      this->start();
    }

    virtual void Cancel()
    {
      Canceled = true;
      this->wait();
    }

    virtual void run()
    {
      Canceled = false;
      OnScanStart();
      for (ItemsDone = 0, ItemsTotal = 0;; ++ItemsDone)
      {
        {
          QMutexLocker lock(&QueueLock);
          if (Queue.empty())
          {
            break;
          }
          ItemsTotal += Queue.size();
          CurrentItem = Queue.takeFirst();
        }

        const Parameters::Accessor::Ptr commonParams = Parameters::Container::Create();
        const String& strPath = FromQString(CurrentItem);
        if (const Error& e = Provider->DetectModules(strPath, commonParams, *this))
        {
          //TODO: check and show error
          e.GetText();
        }
      }
      OnScanStop();
    }
  private:
    virtual bool ProcessPlayitem(Playitem::Ptr item)
    {
      OnGetItem(item);
      return !Canceled;
    }

    virtual void ShowProgress(const Log::MessageData& msg)
    {
      if (0 != msg.Level)
      {
        return;
      }
      if (msg.Progress)
      {
        OnProgress(*msg.Progress, ItemsDone, ItemsTotal);
      }
      if (msg.Text)
      {
        const QString text = ToQString(*msg.Text);
        OnProgressMessage(text, CurrentItem);
      }
    }
  private:
    const PlayitemsProvider::Ptr Provider;
    QMutex QueueLock;
    QStringList Queue;
    //TODO: possibly use events
    volatile bool Canceled;
    unsigned ItemsDone;
    unsigned ItemsTotal;
    QString CurrentItem;
  };
}

ProcessThread* ProcessThread::Create(QObject* owner, PlayitemsProvider::Ptr provider)
{
  assert(owner);
  return new ProcessThreadImpl(owner, provider);
}
