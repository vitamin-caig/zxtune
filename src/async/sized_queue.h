/**
 *
 * @file
 *
 * @brief Limited size implementation of Queue
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <contract.h>
#include <make_ptr.h>
// library includes
#include <async/queue.h>
// std includes
#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>

namespace Async
{
  template<class T>
  class SizedQueue : public Queue<T>
  {
  public:
    explicit SizedQueue(std::size_t maxSize)
      : MaxSize(maxSize)
      , Active(true)
    {}

    void Add(T val) override
    {
      std::unique_lock<std::mutex> lock(Locker);
      CanPutDataEvent.wait(lock, [this]() { return CanPutData(); });
      if (Active)
      {
        Container.emplace_back(std::move(val));
        CanGetDataEvent.notify_one();
      }
    }

    bool Get(T& res) override
    {
      std::unique_lock<std::mutex> lock(Locker);
      CanGetDataEvent.wait(lock, [this]() { return CanGetData(); });
      if (Active)
      {
        Require(!Container.empty());
        res = std::move(Container.front());
        Container.pop_front();
        CanPutDataEvent.notify_one();
        return true;
      }
      return false;
    }

    void Reset() override
    {
      const std::lock_guard<std::mutex> lock(Locker);
      Container.clear();
      Active = false;
      CanGetDataEvent.notify_all();
      CanPutDataEvent.notify_all();
    }

    void Flush() override
    {
      std::unique_lock<std::mutex> lock(Locker);
      CanPutDataEvent.wait(lock, [this]() { return Container.empty(); });
    }

    static typename Queue<T>::Ptr Create(std::size_t size)
    {
      return MakePtr<SizedQueue<T> >(size);
    }

  private:
    bool CanPutData() const
    {
      return !Active || Container.size() < MaxSize;
    }

    bool CanGetData() const
    {
      return !Active || !Container.empty();
    }

  private:
    const std::size_t MaxSize;
    // std::atomic_bool does not work in MSVC
    std::atomic<bool> Active;
    mutable std::mutex Locker;
    std::condition_variable CanPutDataEvent;
    std::condition_variable CanGetDataEvent;
    using ContainerType = std::deque<T>;
    ContainerType Container;
  };
}  // namespace Async
