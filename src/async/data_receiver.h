/**
*
* @file     async/data_receiver.h
* @brief    Data receiver asynchronous adapter
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef ASYNC_PIPE_H_DEFINED
#define ASYNC_PIPE_H_DEFINED

//common includes
#include <contract.h>
#include <data_streaming.h>
//library includes
#include <async/activity.h>
#include <async/progress.h>
#include <async/queue.h>
//std includes
#include <list>

namespace Async
{
  template<class T>
  class DataReceiver : public ::DataReceiver<T>
  {
  public:
    DataReceiver(std::size_t workersCount, std::size_t queueSize, typename ::DataReceiver<T>::Ptr delegate)
      : QueueObject(SizedQueue<T>::Create(queueSize))
      , Statistic(Progress::Create())
      , Delegate(delegate)
    {
      StartAll(workersCount);
    }

    virtual ~DataReceiver()
    {
      QueueObject->Reset();
      WaitAll();
    }

    virtual void ApplyData(const T& data)
    {
      Statistic->Produce(1);
      QueueObject->Add(data);
    }

    virtual void Flush()
    {
      //may not flush queue
      Statistic->WaitForComplete();
      Delegate->Flush();
    }

    static typename ::DataReceiver<T>::Ptr Create(std::size_t workersCount, std::size_t queueSize, typename ::DataReceiver<T>::Ptr delegate)
    {
      return workersCount
        ? boost::make_shared<DataReceiver>(workersCount, queueSize, delegate)
        : delegate;
    }
  private:
    void StartAll(std::size_t count)
    {
      const typename Operation::Ptr op = boost::make_shared<TransceiveOperation>(QueueObject, Statistic, Delegate);
      while (Activities.size() < count)
      {
        const Activity::Ptr act = Activity::Create(op);
        Activities.push_back(act);
      }
    }

    void WaitAll()
    {
      while (!Activities.empty())
      {
        const Activity::Ptr act = Activities.front();
        Activities.pop_front();
        act->Wait();//TODO log error
      }
    }
  private:
    class TransceiveOperation : public Operation
    {
    public:
      TransceiveOperation(typename Queue<T>::Ptr queue, Progress::Ptr stat, typename DataReceiver<T>::Ptr target)
        : QueueObject(queue)
        , Statistic(stat)
        , Target(target)
      {
      }

      virtual Error Prepare()
      {
        return Error();
      }

      virtual Error Execute()
      {
        try
        {
          T val;
          while (QueueObject->Get(val))
          {
            Target->ApplyData(val);
            Statistic->Consume(1);
          }
          return Error();
        }
        catch (const Error& err)
        {
          return err;
        }
      }
    private:
      const typename Queue<T>::Ptr QueueObject;
      const Progress::Ptr Statistic;
      const typename DataReceiver<T>::Ptr Target;
    };
  private:
    const typename Queue<T>::Ptr QueueObject;
    const Progress::Ptr Statistic;
    const typename ::DataReceiver<T>::Ptr Delegate;
    std::list<typename Activity::Ptr> Activities;
  };
}

#endif //ASYNC_PIPE_H_DEFINED
