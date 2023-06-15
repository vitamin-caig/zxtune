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

// std includes
#include <condition_variable>
#include <mutex>

namespace Async
{
  template<class Type>
  class Event
  {
  public:
    explicit Event(Type val)
      : Value(ToMask(val))
    {}

    Event() = default;

    void Set(Type val)
    {
      {
        const std::lock_guard<std::mutex> lock(Mutex);
        Value = ToMask(val);
      }
      Condition.notify_all();
    }

    void Wait(Type val)
    {
      WaitInternal(ToMask(val));
    }

    Type WaitForAny(Type val1, Type val2)
    {
      const unsigned mask1 = ToMask(val1);
      const unsigned mask2 = ToMask(val2);
      const unsigned res = WaitInternal(mask1 | mask2);
      return (res & mask1) ? val1 : val2;
    }

    Type WaitForAny(Type val1, Type val2, Type val3)
    {
      const unsigned mask1 = ToMask(val1);
      const unsigned mask2 = ToMask(val2);
      const unsigned mask3 = ToMask(val3);
      const unsigned res = WaitInternal(mask1 | mask2 | mask3);
      return (res & mask1) ? val1 : ((res & mask2) ? val2 : val3);
    }

    void Reset()
    {
      const std::lock_guard<std::mutex> lock(Mutex);
      Value = 0;
    }

    bool Check(Type val) const
    {
      const std::lock_guard<std::mutex> lock(Mutex);
      return IsSignalled(ToMask(val));
    }

  private:
    static unsigned ToMask(Type val)
    {
      return 1u << static_cast<unsigned>(val);
    }

    unsigned WaitInternal(unsigned mask)
    {
      std::unique_lock<std::mutex> lock(Mutex);
      Condition.wait(lock, [this, mask]() { return IsSignalled(mask); });
      return Value;
    }

    bool IsSignalled(unsigned mask) const
    {
      return 0 != (Value & mask);
    }

  private:
    unsigned Value = 0;
    mutable std::mutex Mutex;
    std::condition_variable Condition;
  };
}  // namespace Async
