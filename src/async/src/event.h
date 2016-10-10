/**
* 
* @file
*
* @brief Event helper class
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//std includes
#include <condition_variable>
#include <mutex>
//boost includes
#include <boost/bind.hpp>

namespace Async
{
  template<class Type>
  class Event
  {
  public:
    explicit Event(Type val)
      : Value(1 << val)
    {
    }

    Event()
      : Value(0)
    {
    }

    void Set(Type val)
    {
      {
        const std::lock_guard<std::mutex> lock(Mutex);
        Value = 1 << val;
      }
      Condition.notify_all();
    }

    void Wait(Type val)
    {
      WaitInternal(1 << val);
    }

    Type WaitForAny(Type val1, Type val2)
    {
      const unsigned mask1 = 1 << val1;
      const unsigned mask2 = 1 << val2;
      const unsigned res = WaitInternal(mask1 | mask2);
      return (res & mask1)
        ? val1
        : val2;
    }

    Type WaitForAny(Type val1, Type val2, Type val3)
    {
      const unsigned mask1 = 1 << val1;
      const unsigned mask2 = 1 << val2;
      const unsigned mask3 = 1 << val3;
      const unsigned res = WaitInternal(mask1 | mask2 | mask3);
      return (res & mask1)
        ? val1
        : ((res & mask2)
          ? val2
          : val3);
    }

    void Reset()
    {
      const std::lock_guard<std::mutex> lock(Mutex);
      Value = 0;
    }

    bool Check(Type val) const
    {
      const std::lock_guard<std::mutex> lock(Mutex);
      return IsSignalled(1 << val);
    }
  private:
    unsigned WaitInternal(unsigned mask)
    {
      std::unique_lock<std::mutex> lock(Mutex);
      Condition.wait(lock, boost::bind(&Event::IsSignalled, this, mask));
      return Value;
    }

    bool IsSignalled(unsigned mask) const
    {
      return 0 != (Value & mask);
    }
  private:
    unsigned Value;
    mutable std::mutex Mutex;
    std::condition_variable Condition;
  };
}
