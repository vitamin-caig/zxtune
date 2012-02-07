/**
*
* @file     async/queue.h
* @brief    Typed thread-safe queue interface
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef ASYNC_QUEUE_H_DEFINED
#define ASYNC_QUEUE_H_DEFINED

//common includes
#include <contract.h>
//std includes
#include <deque>
//boost includes
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>

namespace Async
{
  template<class T>
  class Queue
  {
  public:
    typedef boost::shared_ptr<Queue<T> > Ptr;

    virtual ~Queue() {}

    //! @brief Enqueue object
    virtual void Add(T val) = 0;
    //! @brief Get last available object, block until arrives or break
    //! @return true if object is acquired, else otherwise
    //! @invariant In case of false return res will keep intact
    virtual bool Get(T& res) = 0;
    //! @brief Clear queue and release all the waiters
    virtual void Reset() = 0;
    //! @brief Wait until queue become empty
    virtual void Flush() = 0;
  };

  
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
      return boost::make_shared<SizedQueue<T> >(size);
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
    boost::mutex Locker;
    boost::condition_variable CanPutDataEvent;
    boost::condition_variable CanGetDataEvent;
    typedef std::deque<T> ContainerType;
    ContainerType Container;
  };

  template<class T>
  class UnlimitedQueue : public SizedQueue<T>
  {
  public:
    UnlimitedQueue()
      : SizedQueue<T>(~std::size_t(0))
    {
    }

    static typename Queue<T>::Ptr Create()
    {
      return boost::make_shared<UnlimitedQueue<T> >();
    }
  };

  template<class T>
  class StubQueue : public Queue<T>
  {
  public:
    virtual void Add(T /*val*/)
    {
    }

    virtual bool Get(T& /*res*/)
    {
      return false;
    }

    virtual void Reset()
    {
    }

    virtual void Flush()
    {
    }

    static typename Queue<T>::Ptr Create()
    {
      return boost::make_shared<StubQueue>();
    }
  };
}

#endif //ASYNC_QUEUE_H_DEFINED
