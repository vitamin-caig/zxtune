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
#include <debug_log.h>
#include <error.h>
//library includes
#include <core/error_codes.h>
//qt includes
#include <QtCore/QMutex>
//std includes
#include <ctime>

#define FILE_TAG EA26A866

namespace
{
  const Debug::Stream Dbg("Playlist::Scanner");

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

  class ScannerImpl : public Playlist::Scanner
                    , private Playlist::ScannerCallback
  {
    typedef QList<Playlist::ScannerSource::Ptr> ScannersQueue;
  public:
    ScannerImpl(QObject& parent, Playlist::Item::DataProvider::Ptr provider)
      : Playlist::Scanner(parent)
      , Provider(provider)
      , Canceled(false)
      , ItemsDone()
      , ItemsTotal()
    {
      Dbg("Created at %1%", this);
    }

    virtual ~ScannerImpl()
    {
      Dbg("Destroyed at %1%", this);
    }

    virtual void AddItems(const QStringList& items)
    {
      QMutexLocker lock(&QueueLock);
      if (Canceled)
      {
        this->wait();
      }
      const Playlist::ScannerSource::Ptr scanner = Playlist::ScannerSource::CreateDetectFileSource(Provider, *this, items);
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
      //RAII usage is complicated by On* signals visibility (protected)
      try
      {
        OnScanStart();
        while (Playlist::ScannerSource::Ptr scanner = GetNextScanner())
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
      catch (const Error& err)
      {
        OnScanStop();
        emit ErrorOccurred(err);
      }
    }
  private:
    Playlist::ScannerSource::Ptr GetNextScanner()
    {
      QMutexLocker lock(&QueueLock);
      if (Canceled || Queue.empty())
      {
        return Playlist::ScannerSource::Ptr();
      }
      return Queue.takeFirst();
    }

    virtual bool IsCanceled() const
    {
      return Canceled;
    }

    virtual void OnItem(Playlist::Item::Data::Ptr item)
    {
      OnGetItem(item);
    }

    virtual void OnProgress(unsigned progress, unsigned curItem)
    {
      if (Canceled)
      {
        throw Error(THIS_LINE, ZXTune::Module::ERROR_DETECT_CANCELED);
      }
      if (!StatusFilter())
      {
        OnProgressStatus(progress, ItemsDone + curItem, ItemsTotal);
      }
    }

    virtual void OnReport(const QString& report, const QString& item)
    {
      if (Canceled)
      {
        throw Error(THIS_LINE, ZXTune::Module::ERROR_DETECT_CANCELED);
      }
      if (!MessageFilter())
      {
        OnProgressMessage(report, item);
      }
    }

    virtual void OnError(const Error& err)
    {
      if (err.GetCode() != ZXTune::Module::ERROR_DETECT_CANCELED)
      {
        emit ErrorOccurred(err);
      }
    }
  private:
    const Playlist::Item::DataProvider::Ptr Provider;
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

namespace Playlist
{
  Scanner::Scanner(QObject& parent) : QThread(&parent)
  {
  }

  Scanner::Ptr Scanner::Create(QObject& parent, Playlist::Item::DataProvider::Ptr provider)
  {
    REGISTER_METATYPE(Playlist::Item::Data::Ptr);
    return new ScannerImpl(parent, provider);
  }
}
