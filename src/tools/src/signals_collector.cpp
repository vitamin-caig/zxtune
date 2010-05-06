/*
Abstract:
  Signals working implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//common includes
#include <logging.h>
#include <signals_collector.h>
//std includes
#include <algorithm>
#include <list>
//boost includes
#include <boost/bind.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>

namespace
{
  const std::string THIS_MODULE("Signals");

  class SignalsCollectorImpl : public SignalsCollector
  {
  public:
    typedef boost::shared_ptr<SignalsCollectorImpl> Ptr;
    typedef boost::weak_ptr<SignalsCollectorImpl> WeakPtr;

    explicit SignalsCollectorImpl(uint_t mask)
      : Mask(mask)
      , Signals(0)
    {
    }

    virtual bool WaitForSignals(uint_t& signals, uint_t timeoutMs)
    {
      boost::unique_lock<boost::mutex> locker(Mutex);
      if (Event.timed_wait(locker, boost::posix_time::milliseconds(timeoutMs)))
      {
        assert(Signals);
        signals = Signals;
        Signals = 0;
        return true;
      }
      return false;
    }

    void Notify(uint_t signal)
    {
      if (0 != (signal & Mask))
      {
        boost::unique_lock<boost::mutex> locker(Mutex);
        Signals |= signal;
        Event.notify_one();
      }
    }
  private:
    const uint_t Mask;
    boost::mutex Mutex;
    boost::condition_variable Event;
    uint_t Signals;
  };

  class SignalsDispatcherImpl : public SignalsDispatcher
  {
    typedef boost::lock_guard<boost::mutex> Locker;
  public:
    SignalsDispatcherImpl()
    {
    }

    virtual SignalsCollector::Ptr CreateCollector(uint_t mask) const
    {
      Locker lock(Lock);
      SignalsCollectorImpl::Ptr ptr(new SignalsCollectorImpl(mask));
      Collectors.push_back(CollectorEntry(ptr));
      return ptr;
    }

    virtual void Notify(uint_t signal)
    {
      Locker lock(Lock);
      CollectorEntries::iterator newEnd = std::remove_if(Collectors.begin(), Collectors.end(),
        boost::bind(&SignalsCollectorImpl::WeakPtr::expired, boost::bind(&CollectorEntry::Collector, _1)));
      std::for_each(Collectors.begin(), newEnd, boost::bind(&CollectorEntry::DoSignal, _1, signal));
      Collectors.erase(newEnd, Collectors.end());
    }
  private:
    struct CollectorEntry
    {
      CollectorEntry()
        : Id(0)
      {
      }

      explicit CollectorEntry(SignalsCollectorImpl::Ptr ptr)
        : Collector(ptr)
        , Id(reinterpret_cast<uint32_t>(ptr.get()))
      {
        Log::Debug(THIS_MODULE, "Created collector %1$04x", Id);
      }

      ~CollectorEntry()
      {
        if (0 != Id)
        {
          Log::Debug(THIS_MODULE, "Destroyed collector %1$04x", Id);
        }
      }

      void DoSignal(uint_t signal)
      {
        const SignalsCollectorImpl::Ptr ptr = Collector.lock();
        ptr->Notify(signal);
      }

      SignalsCollectorImpl::WeakPtr Collector;
      uint32_t Id;
    };
    typedef std::list<CollectorEntry> CollectorEntries;
    mutable boost::mutex Lock;
    mutable CollectorEntries Collectors;
  };
}

SignalsDispatcher::Ptr SignalsDispatcher::Create()
{
  return SignalsDispatcher::Ptr(new SignalsDispatcherImpl);
}
