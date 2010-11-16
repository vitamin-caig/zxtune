/*
Abstract:
  Playlist scanner implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "scanner.h"
#include "source.h"
#include "ui/utils.h"
//common includes
#include <error.h>
//qt includes
#include <QtCore/QMutex>
//std includes
#include <ctime>

namespace
{
  class EventFilter
  {
  public:
    EventFilter()
      : LastTime(0)
    {
    }

    bool operator()()
    {
      const std::time_t curTime = ::std::time(0);
      if (curTime == LastTime)
      {
        return true;
      }
      LastTime = curTime;
      return false;
    }
  private:
    std::time_t LastTime;
  };

  class PlaylistScannerImpl : public PlaylistScanner
                            , private ScannerCallback
  {
    typedef QList<ScannerSource::Ptr> ScannersQueue;
  public:
    PlaylistScannerImpl(QObject& parent, PlayitemsProvider::Ptr provider)
      : PlaylistScanner(parent)
      , Provider(provider)
      , Canceled(false)
      , ItemsDone()
      , ItemsTotal()
    {
    }

    virtual void AddItems(const QStringList& items, bool deepScan)
    {
      QMutexLocker lock(&QueueLock);
      if (Canceled)
      {
        this->wait();
      }
      const ScannerSource::Ptr scanner = deepScan
        ? ScannerSource::CreateDetectFileSource(Provider, *this, items)
        : ScannerSource::CreateOpenFileSource(Provider, *this, items);
      Queue.append(scanner);
      this->start();
    }

    virtual void AddItems(Playitem::Iterator::Ptr items, int countHint)
    {
      QMutexLocker lock(&QueueLock);
      if (Canceled)
      {
        this->wait();
      }
      const ScannerSource::Ptr scanner = ScannerSource::CreateIteratorSource(*this, items);
      Queue.append(scanner);
      this->start();
    }

    virtual void Cancel()
    {
      QMutexLocker lock(&QueueLock);
      Canceled = true;
      Queue.clear();
    }

    virtual void run()
    {
      Canceled = false;
      ItemsDone = ItemsTotal = 0;
      OnScanStart();
      while (ScannerSource::Ptr scanner = GetNextScanner())
      {
        OnResolvingStart();
        const unsigned newItems = scanner->Resolve();
        OnResolvingStop();
        ItemsTotal += newItems;
        scanner->Process();
        ItemsDone += newItems;
      }
      OnScanStop();
    }
  private:
    ScannerSource::Ptr GetNextScanner()
    {
      QMutexLocker lock(&QueueLock);
      if (Canceled || Queue.empty())
      {
        return ScannerSource::Ptr();
      }
      return Queue.takeFirst();
    }

    virtual bool IsCanceled() const
    {
      return Canceled;
    }

    virtual void OnPlayitem(const Playitem::Ptr& item)
    {
      OnGetItem(item);
    }

    virtual void OnProgress(unsigned progress, unsigned curItem)
    {
      if (!StatusFilter())
      {
        OnProgressStatus(progress, ItemsDone + curItem, ItemsTotal);
      }
    }

    virtual void OnReport(const QString& report, const QString& item)
    {
      if (!MessageFilter())
      {
        OnProgressMessage(report, item);
      }
    }

    virtual void OnError(const Error& err)
    {
      //TODO
    }
  private:
    const PlayitemsProvider::Ptr Provider;
    QMutex QueueLock;
    ScannersQueue Queue;
    //TODO: possibly use events
    volatile bool Canceled;
    unsigned ItemsDone;
    unsigned ItemsTotal;
    EventFilter StatusFilter;
    EventFilter MessageFilter;
  };
}

PlaylistScanner::PlaylistScanner(QObject& parent) : QThread(&parent)
{
}

PlaylistScanner* PlaylistScanner::Create(QObject& parent, PlayitemsProvider::Ptr provider)
{
  return new PlaylistScannerImpl(parent, provider);
}
