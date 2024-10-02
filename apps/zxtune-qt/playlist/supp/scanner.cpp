/**
 *
 * @file
 *
 * @brief Scanner implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "scanner.h"
#include "playlist/io/import.h"
#include "ui/utils.h"
// common includes
#include <contract.h>
#include <error.h>
#include <make_ptr.h>
#include <string_view.h>
// library includes
#include <async/coroutine.h>
#include <debug/log.h>
#include <time/elapsed.h>
// std includes
#include <memory>
#include <mutex>
// qt includes
#include <QtCore/QDirIterator>
#include <QtCore/QStringList>
#include <utility>

namespace
{
  const Debug::Stream Dbg("Playlist::Scanner");

  class FilenamesTarget
  {
  public:
    virtual ~FilenamesTarget() = default;

    virtual void Add(const QStringList& items) = 0;
  };

  class FilenamesSource
  {
  public:
    virtual ~FilenamesSource() = default;

    virtual bool Empty() const = 0;
    virtual QString GetNext() = 0;
  };

  class SourceFilenames
    : public FilenamesTarget
    , public FilenamesSource
  {
  public:
    void Add(const QStringList& items) override
    {
      Items.append(items);
    }

    bool Empty() const override
    {
      return Items.isEmpty();
    }

    QString GetNext() override
    {
      Require(!Items.isEmpty());
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
    {}

    bool Empty() const override
    {
      return NoCurrentDir() && Delegate.Empty();
    }

    QString GetNext() override
    {
      while (NoCurrentDir())
      {
        auto res = Delegate.GetNext();
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
      return nullptr == CurDir || !CurDir->hasNext();
    }

    static bool IsDir(const QString& name)
    {
      return QFileInfo(name).isDir();
    }

    void OpenDir(const QString& name)
    {
      const QDir::Filters filter = QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden;
      CurDir = std::make_unique<QDirIterator>(name, filter, QDirIterator::Subdirectories);
    }

    void CloseDir()
    {
      CurDir.reset(nullptr);
    }

  private:
    FilenamesSource& Delegate;
    std::unique_ptr<QDirIterator> CurDir;
  };

  class PrefetchedFilenames : public FilenamesSource
  {
  public:
    PrefetchedFilenames(FilenamesSource& delegate, int lowerBound, int upperBound)
      : Delegate(delegate)
      , LowerBound(lowerBound)
      , UpperBound(upperBound)
    {}

    bool Empty() const override
    {
      return Cache.isEmpty() && Delegate.Empty();
    }

    QString GetNext() override
    {
      Prefetch();
      Require(!Cache.isEmpty());
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
    {}

    bool Empty() const override
    {
      return Delegate.Empty();
    }

    QString GetNext() override
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
    unsigned GotFilenames = 0;
  };

  class FilesQueue
    : public FilenamesTarget
    , public FilenamesSource
    , public Playlist::ScanStatus
  {
  public:
    using Ptr = std::shared_ptr<FilesQueue>;

    FilesQueue()
      : Resolved(Source)
      , ResolvedStatistic(Resolved)
      , Prefetched(ResolvedStatistic, 500, 1000)
      , PrefetchedStatistic(Prefetched)
    {}

    void Add(const QStringList& items) override
    {
      const std::lock_guard<std::mutex> lock(Lock);
      Source.Add(items);
    }

    // FilenamesSource
    bool Empty() const override
    {
      return Prefetched.Empty();
    }

    QString GetNext() override
    {
      const std::lock_guard<std::mutex> lock(Lock);
      return Current = PrefetchedStatistic.GetNext();
    }

    // ScanStatus
    unsigned DoneFiles() const override
    {
      return PrefetchedStatistic.Get();
    }

    unsigned FoundFiles() const override
    {
      return ResolvedStatistic.Get();
    }

    QString CurrentFile() const override
    {
      return Current;
    }

    bool SearchFinished() const override
    {
      return Resolved.Empty();
    }

  private:
    std::mutex Lock;
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
    virtual ~ScannerCallback() = default;

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
    {}

    Parameters::Container::Ptr CreateInitialAdjustedParameters() const override
    {
      return Parameters::Container::Create();
    }

    void ProcessItem(Playlist::Item::Data::Ptr item) override
    {
      Callback.OnItem(item);
    }

    Log::ProgressCallback* GetProgress() const override
    {
      return &Progress;
    }

  private:
    ScannerCallback& Callback;
    Log::ProgressCallback& Progress;
  };

  const auto UI_NOTIFICATION_PERIOD = Time::Milliseconds(500);

  class ProgressCallbackAdapter : public Log::ProgressCallback
  {
  public:
    ProgressCallbackAdapter(ScannerCallback& cb, Async::Scheduler& sched)
      : Callback(cb)
      , Scheduler(sched)
      , ReportTimeout(UI_NOTIFICATION_PERIOD)
    {}

    void OnProgress(uint_t current) override
    {
      if (ReportTimeout())
      {
        Callback.OnProgress(current);
        Scheduler.Yield();
      }
    }

    void OnProgress(uint_t current, StringView message) override
    {
      if (ReportTimeout())
      {
        Callback.OnProgress(current);
        Callback.OnMessage(ToQString(message));
        Scheduler.Yield();
      }
    }

  private:
    ScannerCallback& Callback;
    Async::Scheduler& Scheduler;
    Time::Elapsed ReportTimeout;
  };

  class ScanRoutine
    : public FilenamesTarget
    , public Async::Coroutine
  {
  public:
    using Ptr = std::shared_ptr<ScanRoutine>;

    ScanRoutine(ScannerCallback& cb, Playlist::Item::DataProvider::Ptr provider)
      : Callback(cb)
      , Provider(std::move(provider))
    {
      CreateQueue();
    }

    void Add(const QStringList& items) override
    {
      Queue->Add(items);
    }

    void Initialize() override
    {
      Callback.OnScanStart(Queue);
    }

    void Finalize() override
    {
      Callback.OnScanEnd();
      CreateQueue();
    }

    void Suspend() override {}

    void Resume() override {}

    void Execute(Async::Scheduler& sched) override
    {
      ProgressCallbackAdapter cb(Callback, sched);
      while (!Queue->Empty())
      {
        try
        {
          const QString file = Queue->GetNext();
          ScanFile(file, cb);
        }
        catch (const std::exception&)
        {}
      }
    }

  private:
    void CreateQueue()
    {
      Queue = MakePtr<FilesQueue>();
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

  class ScannerImpl
    : public Playlist::Scanner
    , private ScannerCallback
  {
  public:
    ScannerImpl(QObject& parent, Playlist::Item::DataProvider::Ptr provider)
      : Playlist::Scanner(parent)
      , Provider(provider)
      , Routine(MakePtr<ScanRoutine>(static_cast<ScannerCallback&>(*this), std::move(provider)))
      , ScanJob(Async::CreateJob(Routine))
    {
      Dbg("Created at {}", Self());
    }

    ~ScannerImpl() override
    {
      Dbg("Destroyed at {}", Self());
    }

    void AddItems(const QStringList& items) override
    {
      Dbg("Added {} items to {}", items.size(), Self());
      Routine->Add(items);
      ScanJob->Start();
    }

    void PasteItems(const QStringList& items) override
    {
      Dbg("Paste {} items to {}", items.size(), Self());
      // TODO: optimize
      const Playlist::IO::Container::Ptr container = Playlist::IO::OpenPlainList(Provider, items);
      emit ItemsFound(container->GetItems());
    }

    void Pause(bool pause) override
    {
      if (pause)
      {
        Dbg("Pausing {}", Self());
        ScanJob->Pause();
      }
      else
      {
        Dbg("Resuming {}", Self());
        ScanJob->Start();
      }
    }

    void Stop() override
    {
      Dbg("Stopping {}", Self());
      ScanJob->Stop();
    }

  private:
    const void* Self() const
    {
      return this;
    }

    void OnItem(Playlist::Item::Data::Ptr item) override
    {
      emit ItemFound(item);
    }

    void OnItems(Playlist::Item::Collection::Ptr items) override
    {
      emit ItemsFound(items);
    }

    void OnScanStart(Playlist::ScanStatus::Ptr status) override
    {
      emit ScanStarted(status);
    }

    void OnProgress(unsigned progress) override
    {
      emit ScanProgressChanged(progress);
    }

    void OnMessage(const QString& message) override
    {
      emit ScanMessageChanged(message);
    }

    void OnError(const Error& err) override
    {
      emit ErrorOccurred(err);
    }

    void OnScanEnd() override
    {
      emit ScanStopped();
    }

  private:
    const Playlist::Item::DataProvider::Ptr Provider;
    const ScanRoutine::Ptr Routine;
    const Async::Job::Ptr ScanJob;
  };
}  // namespace

namespace Playlist
{
  Scanner::Scanner(QObject& parent)
    : QObject(&parent)
  {}

  Scanner::Ptr Scanner::Create(QObject& parent, Playlist::Item::DataProvider::Ptr provider)
  {
    REGISTER_METATYPE(Playlist::Item::Data::Ptr);
    REGISTER_METATYPE(Playlist::Item::Collection::Ptr);
    REGISTER_METATYPE(Playlist::ScanStatus::Ptr);
    return new ScannerImpl(parent, std::move(provider));
  }
}  // namespace Playlist
