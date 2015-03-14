/**
* 
* @file
*
* @brief Scanner implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "scanner.h"
#include "playlist/io/import.h"
#include "ui/utils.h"
//common includes
#include <error.h>
//library includes
#include <async/coroutine.h>
#include <debug/log.h>
#include <time/elapsed.h>
//boost includes
#include <boost/make_shared.hpp>
#include <boost/ref.hpp>
#include <boost/thread/mutex.hpp>
//qt includes
#include <QtCore/QDirIterator>
#include <QtCore/QStringList>

#define FILE_TAG EA26A866

namespace
{
  const Debug::Stream Dbg("Playlist::Scanner");

  class FilenamesTarget
  {
  public:
    virtual ~FilenamesTarget() {}

    virtual void Add(const QStringList& items) = 0;
  };

  class FilenamesSource
  {
  public:
    virtual ~FilenamesSource() {}

    virtual bool Empty() const = 0;
    virtual QString GetNext() = 0;
  };

  class SourceFilenames : public FilenamesTarget
                        , public FilenamesSource
  {
  public:
    virtual void Add(const QStringList& items)
    {
      Items.append(items);
    }

    virtual bool Empty() const
    {
      return Items.isEmpty();
    }

    virtual QString GetNext()
    {
      return Items.takeFirst();
    }
  private:
    QStringList Items;
  };

  class ResolvedFilenames : public FilenamesSource
  {
  public:
    explicit ResolvedFilenames(FilenamesSource& delegate)
      : Delegate(delegate)
    {
    }

    virtual bool Empty() const
    {
      return NoCurrentDir() && Delegate.Empty();
    }

    virtual QString GetNext()
    {
      while (NoCurrentDir())
      {
        const QString res = Delegate.GetNext();
        if (IsDir(res))
        {
          OpenDir(res);
          continue;
        }
        else
        {
          CloseDir();
          return res;
        }
      }
      return CurDir->next();
    }
  private:
    bool NoCurrentDir() const
    {
      return 0 == CurDir.get() || !CurDir->hasNext();
    }

    static bool IsDir(const QString& name)
    {
      return QFileInfo(name).isDir();
    }

    void OpenDir(const QString& name)
    {
      const QDir::Filters filter = QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden;
      CurDir.reset(new QDirIterator(name, filter, QDirIterator::Subdirectories));
    }

    void CloseDir()
    {
      CurDir.reset(0);
    }
  private:
    FilenamesSource& Delegate;
    std::auto_ptr<QDirIterator> CurDir;
  };

  class PrefetchedFilenames : public FilenamesSource
  {
  public:
    PrefetchedFilenames(FilenamesSource& delegate, int lowerBound, int upperBound)
      : Delegate(delegate)
      , LowerBound(lowerBound)
      , UpperBound(upperBound)
    {
    }

    virtual bool Empty() const
    {
      return Cache.isEmpty() && Delegate.Empty();
    }

    virtual QString GetNext()
    {
      Prefetch();
      return Cache.takeFirst();
    }
  private:
    void Prefetch()
    {
      int curSize = Cache.size();
      if (curSize < LowerBound && !Delegate.Empty())
      {
        for (; curSize < UpperBound && !Delegate.Empty(); ++curSize)
        {
          Cache.append(Delegate.GetNext());
        }
      }
    }
  private:
    FilenamesSource& Delegate;
    const int LowerBound;
    const int UpperBound;
    QStringList Cache;
  };

  class FilenamesSourceStatistic : public FilenamesSource
  {
  public:
    FilenamesSourceStatistic(FilenamesSource& delegate)
      : Delegate(delegate)
      , GotFilenames(0)
    {
    }

    virtual bool Empty() const
    {
      return Delegate.Empty();
    }

    virtual QString GetNext()
    {
      ++GotFilenames;
      return Delegate.GetNext();
    }

    unsigned Get() const
    {
      return GotFilenames;
    }
  private:
    FilenamesSource& Delegate;
    unsigned GotFilenames;
  };

  class FilesQueue : public FilenamesTarget
                   , public FilenamesSource
                   , public Playlist::ScanStatus
  {
  public:
    typedef boost::shared_ptr<FilesQueue> Ptr;

    FilesQueue()
      : Resolved(Source)
      , ResolvedStatistic(Resolved)
      , Prefetched(ResolvedStatistic, 500, 1000)
      , PrefetchedStatistic(Prefetched)
    {
    }

    virtual void Add(const QStringList& items)
    {
      const boost::mutex::scoped_lock lock(Lock);
      Source.Add(items);
    }

    //FilenamesSource
    virtual bool Empty() const
    {
      return Prefetched.Empty();
    }

    virtual QString GetNext()
    {
      const boost::mutex::scoped_lock lock(Lock);
      return Current = PrefetchedStatistic.GetNext();
    }

    //ScanStatus
    virtual unsigned DoneFiles() const
    {
      return PrefetchedStatistic.Get();
    }

    virtual unsigned FoundFiles() const
    {
      return ResolvedStatistic.Get();
    }

    virtual QString CurrentFile() const
    {
      return Current;
    }

    virtual bool SearchFinished() const
    {
      return Resolved.Empty();
    }
  private:
    boost::mutex Lock;
    SourceFilenames Source;
    ResolvedFilenames Resolved;
    FilenamesSourceStatistic ResolvedStatistic;
    PrefetchedFilenames Prefetched;
    FilenamesSourceStatistic PrefetchedStatistic;
    QString Current;
  };

  class ScannerCallback
  {
  public:
    virtual ~ScannerCallback() {}

    virtual void OnItem(Playlist::Item::Data::Ptr item) = 0;
    virtual void OnItems(Playlist::Item::Collection::Ptr items) = 0;
    virtual void OnScanStart(Playlist::ScanStatus::Ptr status) = 0;
    virtual void OnProgress(unsigned progress) = 0;
    virtual void OnMessage(const QString& message) = 0;
    virtual void OnError(const class Error& err) = 0;
    virtual void OnScanEnd() = 0;
  };

  class DetectParamsAdapter : public Playlist::Item::DetectParameters
  {
  public:
    DetectParamsAdapter(ScannerCallback& cb, Log::ProgressCallback& progress)
      : Callback(cb)
      , Progress(progress)
    {
    }

    virtual Parameters::Container::Ptr CreateInitialAdjustedParameters() const
    {
      return Parameters::Container::Create();
    }

    virtual void ProcessItem(Playlist::Item::Data::Ptr item)
    {
      Callback.OnItem(item);
    }

    virtual Log::ProgressCallback* GetProgress() const
    {
      return &Progress;
    }
  private:
    ScannerCallback& Callback;
    Log::ProgressCallback& Progress;
  };

  const Time::Milliseconds UI_NOTIFICATION_PERIOD(500);

  class ProgressCallbackAdapter : public Log::ProgressCallback
  {
  public:
    ProgressCallbackAdapter(ScannerCallback& cb, Async::Scheduler& sched)
      : Callback(cb)
      , Scheduler(sched)
      , ReportTimeout(UI_NOTIFICATION_PERIOD)
    {
    }

    virtual void OnProgress(uint_t current)
    {
      if (ReportTimeout())
      {
        Callback.OnProgress(current);
        Scheduler.Yield();
      }
    }

    virtual void OnProgress(uint_t current, const String& message)
    {
      if (ReportTimeout())
      {
        Callback.OnProgress(current);
        Callback.OnMessage(ToQStringFromLocal(message));
        Scheduler.Yield();
      }
    }
  private:
    ScannerCallback& Callback;
    Async::Scheduler& Scheduler;
    Time::Elapsed ReportTimeout;
  };

  class ScanRoutine : public FilenamesTarget
                    , public Async::Coroutine
  {
  public:
    typedef boost::shared_ptr<ScanRoutine> Ptr;

    ScanRoutine(ScannerCallback& cb, Playlist::Item::DataProvider::Ptr provider)
      : Callback(cb)
      , Provider(provider)
    {
      CreateQueue();
    }

    virtual void Add(const QStringList& items)
    {
      Queue->Add(items);
    }

    virtual void Initialize()
    {
      Callback.OnScanStart(Queue);
    }

    virtual void Finalize()
    {
      Callback.OnScanEnd();
      CreateQueue();
    }

    virtual void Suspend()
    {
    }

    virtual void Resume()
    {
    }

    virtual void Execute(Async::Scheduler& sched)
    {
      ProgressCallbackAdapter cb(Callback, sched);
      while (!Queue->Empty())
      {
        const QString file = Queue->GetNext();
        ScanFile(file, cb);
      }
    }
  private:
    void CreateQueue()
    {
      Queue = boost::make_shared<FilesQueue>();
    }

    void ScanFile(const QString& name, Log::ProgressCallback& cb)
    {
      if (!ProcessAsPlaylist(name, cb))
      {
        DetectSubitems(name, cb);
      }
    }

    bool ProcessAsPlaylist(const QString& path, Log::ProgressCallback& cb)
    {
      try
      {
        const Playlist::IO::Container::Ptr playlist = Playlist::IO::Open(Provider, path, cb);
        if (!playlist.get())
        {
          return false;
        }
        Callback.OnItems(playlist->GetItems());
      }
      catch (const Error& e)
      {
        Callback.OnError(e);
      }
      return true;
    }

    void DetectSubitems(const QString& itemPath, Log::ProgressCallback& cb)
    {
      try
      {
        DetectParamsAdapter params(Callback, cb);
        Provider->DetectModules(FromQString(itemPath), params);
      }
      catch (const Error& e)
      {
        Callback.OnError(e);
      }
    }
  private:
    ScannerCallback& Callback;
    const Playlist::Item::DataProvider::Ptr Provider;
    FilesQueue::Ptr Queue;
  };

  class ScannerImpl : public Playlist::Scanner
                    , private ScannerCallback
  {
  public:
    ScannerImpl(QObject& parent, Playlist::Item::DataProvider::Ptr provider)
      : Playlist::Scanner(parent)
      , Provider(provider)
      , Routine(boost::make_shared<ScanRoutine>(boost::ref(static_cast<ScannerCallback&>(*this)), provider))
      , ScanJob(Async::CreateJob(Routine))
    {
      Dbg("Created at %1%", this);
    }

    virtual ~ScannerImpl()
    {
      Dbg("Destroyed at %1%", this);
    }

    virtual void AddItems(const QStringList& items)
    {
      Dbg("Added %1% items to %2%", items.size(), this);
      Routine->Add(items);
      ScanJob->Start();
    }

    virtual void PasteItems(const QStringList& items)
    {
      Dbg("Paste %1% items to %2%", items.size(), this);
      //TODO: optimize
      const Playlist::IO::Container::Ptr container = Playlist::IO::OpenPlainList(Provider, items);
      emit ItemsFound(container->GetItems());
    }

    virtual void Pause(bool pause)
    {
      Dbg(pause ? "Pausing %1%" : "Resuming %1%", this);
      if (pause)
      {
        ScanJob->Pause();
      }
      else
      {
        ScanJob->Start();
      }
    }

    virtual void Stop()
    {
      Dbg("Stopping %1%", this);
      ScanJob->Stop();
    }
  private:
    virtual void OnItem(Playlist::Item::Data::Ptr item)
    {
      emit ItemFound(item);
    }

    virtual void OnItems(Playlist::Item::Collection::Ptr items)
    {
      emit ItemsFound(items);
    }

    virtual void OnScanStart(Playlist::ScanStatus::Ptr status)
    {
      emit ScanStarted(status);
    }

    virtual void OnProgress(unsigned progress)
    {
      emit ScanProgressChanged(progress);
    }

    virtual void OnMessage(const QString& message)
    {
      emit ScanMessageChanged(message);
    }

    virtual void OnError(const Error& err)
    {
      emit ErrorOccurred(err);
    }

    virtual void OnScanEnd()
    {
      emit ScanStopped();
    }
  private:
    const Playlist::Item::DataProvider::Ptr Provider;
    const ScanRoutine::Ptr Routine;
    const Async::Job::Ptr ScanJob;
  };
}

namespace Playlist
{
  Scanner::Scanner(QObject& parent) : QObject(&parent)
  {
  }

  Scanner::Ptr Scanner::Create(QObject& parent, Playlist::Item::DataProvider::Ptr provider)
  {
    REGISTER_METATYPE(Playlist::Item::Data::Ptr);
    REGISTER_METATYPE(Playlist::Item::Collection::Ptr);
    REGISTER_METATYPE(Playlist::ScanStatus::Ptr);
    return new ScannerImpl(parent, provider);
  }
}
