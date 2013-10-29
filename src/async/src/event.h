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

//boost includes
#include <boost/bind.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>

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
        boost::mutex::scoped_lock lock(Mutex);
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
      boost::mutex::scoped_lock lock(Mutex);
      Value = 0;
    }

    bool Check(Type val) const
    {
      boost::mutex::scoped_lock lock(Mutex);
      return IsSignalled(1 << val);
    }
  private:
    unsigned WaitInternal(unsigned mask)
    {
      boost::mutex::scoped_lock lock(Mutex);
      Condition.wait(lock, boost::bind(&Event::IsSignalled, this, mask));
      return Value;
    }

    bool IsSignalled(unsigned mask) const
    {
      return 0 != (Value & mask);
    }
  private:
    unsigned Value;
    mutable boost::mutex Mutex;
    boost::condition_variable Condition;
  };
}
