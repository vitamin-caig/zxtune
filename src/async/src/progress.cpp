/**
* 
* @file
*
* @brief Progress implementation
*
* @author vitamin.caig@gmail.com
*
**/

//common includes
#include <make_ptr.h>
//library includes
#include <async/progress.h>
//std includes
#include <condition_variable>
#include <mutex>

namespace Async
{
  class SynchronizedProgress : public Progress
  {
  public:
    SynchronizedProgress()
      : Produced()
      , Consumed()
    {
    }

    virtual void Produce(uint_t items)
    {
      const std::lock_guard<std::mutex> lock(Mutex);
      Produced += items;
    }

    virtual void Consume(uint_t items)
    {
      const std::lock_guard<std::mutex> lock(Mutex);
      Consumed += items;
      NotifyIfComplete();
    }

    virtual void WaitForComplete() const
    {
      std::unique_lock<std::mutex> lock(Mutex);
      Complete.wait(lock, [this] () {return IsComplete();});
    }
  private:
    void NotifyIfComplete()
    {
      if (IsComplete())
      {
        Complete.notify_all();
      }
    }

    bool IsComplete() const
    {
      return Produced == Consumed;
    }
  private:
    uint_t Produced;
    uint_t Consumed;
    mutable std::mutex Mutex;
    mutable std::condition_variable Complete;
  };
}

namespace Async
{
  Progress::Ptr Progress::Create()
  {
    return MakePtr<SynchronizedProgress>();
  }
}
