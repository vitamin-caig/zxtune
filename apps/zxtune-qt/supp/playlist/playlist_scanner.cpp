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
#include "playlist_scanner.h"
#include "ui/utils.h"
//common includes
#include <error.h>
#include <logging.h>
//qt includes
#include <QtCore/QDirIterator>
#include <QtCore/QFileInfo>
#include <QtCore/QMutex>
#include <QtCore/QStringList>
//std includes
#include <ctime>

namespace
{
  const std::string THIS_MODULE("UI::PlaylistScanner");

  class ScannerCallback
  {
  public:
    virtual ~ScannerCallback() {}

    virtual bool IsCanceled() const = 0;

    virtual void OnPlayitem(const Playitem::Ptr& item) = 0;
    virtual void OnProgress(unsigned progress, unsigned curItem) = 0;
    virtual void OnReport(const QString& report, const QString& item) = 0;
    virtual void OnError(const Error& err) = 0;
  };

  class Scanner
  {
  public:
    typedef boost::shared_ptr<Scanner> Ptr;

    virtual ~Scanner() {}

    virtual unsigned Resolve() = 0;
    virtual void Process() = 0;

    static Ptr CreateOpener(PlayitemsProvider& provider, ScannerCallback& callback, const QStringList& items);
    static Ptr CreateDetector(PlayitemsProvider& provider, ScannerCallback& callback, const QStringList& items);
  };

  void ResolveItems(ScannerCallback& callback, QStringList& unresolved, QStringList& resolved)
  {
    while (!callback.IsCanceled() && !unresolved.empty())
    {
      const QString& curItem = unresolved.takeFirst();
      if (QFileInfo(curItem).isDir())
      {
        for (QDirIterator iterator(curItem, QDir::Files, QDirIterator::Subdirectories); !callback.IsCanceled() && iterator.hasNext(); )
        {
          resolved.append(iterator.next());
        }
      }
      else
      {
        resolved.append(curItem);
      }
    }
  }

  class DetectParametersWrapper : public PlayitemDetectParameters
  {
  public:
    DetectParametersWrapper(ScannerCallback& callback, unsigned curItemNum, const QString& curItem)
      : Callback(callback)
      , CurrentItemNum(curItemNum)
      , CurrentItem(curItem)
    {
    }

    virtual bool ProcessPlayitem(Playitem::Ptr item)
    {
      Callback.OnPlayitem(item);
      return !Callback.IsCanceled();
    }

    virtual void ShowProgress(const Log::MessageData& msg)
    {
      if (0 != msg.Level)
      {
        return;
      }
      if (msg.Text)
      {
        const QString text = ToQString(*msg.Text);
        Log::Debug(THIS_MODULE, "Scanning %1%", *msg.Text);
        Callback.OnReport(text, CurrentItem);
      }
      if (msg.Progress)
      {
        const uint_t curProgress = *msg.Progress;
        Log::Debug(THIS_MODULE, "Progress %1%", curProgress);
        Callback.OnProgress(curProgress, CurrentItemNum);
      }
    }
  private:
    ScannerCallback& Callback;
    const unsigned CurrentItemNum;
    const QString& CurrentItem;
  };

  class OpenScanner : public Scanner
  {
  public:
    OpenScanner(PlayitemsProvider& provider, ScannerCallback& callback, const QStringList& items)
      : Provider(provider)
      , Callback(callback)
      , UnresolvedItems(items)
    {
    }

    virtual unsigned Resolve()
    {
      ResolveItems(Callback, UnresolvedItems, ResolvedItems);
      return ResolvedItems.size();
    }

    virtual void Process()
    {
      const unsigned totalItems = ResolvedItems.size();
      for (unsigned curItemNum = 0; !ResolvedItems.empty() && !Callback.IsCanceled(); ++curItemNum)
      {
        const QString& curItem = ResolvedItems.takeFirst();
        OpenItem(curItem, curItemNum);
        Callback.OnProgress(curItemNum * 100 / totalItems, curItemNum);
        Callback.OnReport(QString(), curItem);
      }
    }
  private:
    void OpenItem(const QString& itemPath, unsigned itemNum)
    {
      const String& strPath = FromQString(itemPath);
      const Parameters::Accessor::Ptr itemParams = Parameters::Container::Create();
      DetectParametersWrapper detectParams(Callback, itemNum, itemPath);
      if (const Error& e = Provider.OpenModule(strPath, itemParams, detectParams))
      {
        Callback.OnError(e);
      }
    }
  private:
    PlayitemsProvider& Provider;
    ScannerCallback& Callback;
    QStringList UnresolvedItems;
    QStringList ResolvedItems;
  };

  class DetectScanner : public Scanner
  {
  public:
    DetectScanner(PlayitemsProvider& provider, ScannerCallback& callback, const QStringList& items)
      : Provider(provider)
      , Callback(callback)
      , UnresolvedItems(items)
    {
    }

    virtual unsigned Resolve()
    {
      ResolveItems(Callback, UnresolvedItems, ResolvedItems);
      return ResolvedItems.size();
    }

    virtual void Process()
    {
      for (unsigned curItemNum = 0; !ResolvedItems.empty() && !Callback.IsCanceled(); ++curItemNum)
      {
        const QString& curItem = ResolvedItems.takeFirst();
        DetectSubitems(curItem, curItemNum);
      }
    }
  private:
    void DetectSubitems(const QString& itemPath, unsigned itemNum)
    {
      const String& strPath = FromQString(itemPath);
      const Parameters::Accessor::Ptr itemParams = Parameters::Container::Create();
      DetectParametersWrapper detectParams(Callback, itemNum, itemPath);
      if (const Error& e = Provider.DetectModules(strPath, itemParams, detectParams))
      {
        Callback.OnError(e);
      }
    }
  private:
    PlayitemsProvider& Provider;
    ScannerCallback& Callback;
    QStringList UnresolvedItems;
    QStringList ResolvedItems;
  };


  Scanner::Ptr Scanner::CreateOpener(PlayitemsProvider& provider, ScannerCallback& callback, const QStringList& items)
  {
    return Scanner::Ptr(new OpenScanner(provider, callback, items));
  }

  Scanner::Ptr Scanner::CreateDetector(PlayitemsProvider& provider, ScannerCallback& callback, const QStringList& items)
  {
    return Scanner::Ptr(new DetectScanner(provider, callback, items));
  }

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
    typedef QList<Scanner::Ptr> ScannersQueue;
  public:
    PlaylistScannerImpl(QObject* owner, PlayitemsProvider::Ptr provider)
      : Provider(provider)
      , Canceled(false)
      , ItemsDone()
      , ItemsTotal()
    {
      setParent(owner);
    }

    virtual void AddItems(const QStringList& items, bool deepScan)
    {
      QMutexLocker lock(&QueueLock);
      if (Canceled)
      {
        this->wait();
      }
      const Scanner::Ptr scanner = deepScan
        ? Scanner::CreateDetector(*Provider, *this, items)
        : Scanner::CreateOpener(*Provider, *this, items);
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
      while (Scanner::Ptr scanner = GetNextScanner())
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
    Scanner::Ptr GetNextScanner()
    {
      QMutexLocker lock(&QueueLock);
      if (Canceled || Queue.empty())
      {
        return Scanner::Ptr();
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

PlaylistScanner* PlaylistScanner::Create(QObject* owner, PlayitemsProvider::Ptr provider)
{
  assert(owner);
  return new PlaylistScannerImpl(owner, provider);
}
