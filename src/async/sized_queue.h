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

//common includes
#include <contract.h>
#include <make_ptr.h>
//library includes
#include <async/queue.h>
//std includes
#include <deque>
//boost includes
#include <boost/bind.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>

namespace Async
{
  template<class T>
  class SizedQueue : public Queue<T>
  {
  public:
    explicit SizedQueue(std::size_t maxSize)
      : MaxSize(maxSize)
      , Active(true)
    {
    }

    virtual void Add(T val)
    {
      boost::mutex::scoped_lock lock(Locker);
      CanPutDataEvent.wait(lock, boost::bind(&SizedQueue::CanPutData, this));
      if (Active)
      {
        Container.push_back(val);
        CanGetDataEvent.notify_one();
      }
    }

    virtual bool Get(T& res)
    {
      boost::mutex::scoped_lock lock(Locker);
      CanGetDataEvent.wait(lock, boost::bind(&SizedQueue::CanGetData, this));
      if (Active)
      {
        Require(!Container.empty());
        res = Container.front();
        Container.pop_front();
        CanPutDataEvent.notify_one();
        return true;
      }
      return false;
    }

    virtual void Reset()
    {
      const boost::mutex::scoped_lock lock(Locker);
      Container.clear();
      Active = false;
      CanGetDataEvent.notify_all();
      CanPutDataEvent.notify_all();
    }

    virtual void Flush()
    {
      boost::mutex::scoped_lock lock(Locker);
      CanPutDataEvent.wait(lock, boost::bind(&ContainerType::empty, &Container));
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
    volatile bool Active;
    mutable boost::mutex Locker;
    boost::condition_variable CanPutDataEvent;
    boost::condition_variable CanGetDataEvent;
    typedef std::deque<T> ContainerType;
    ContainerType Container;
  };
}
