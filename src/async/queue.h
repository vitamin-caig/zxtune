/**
 *
 * @file
 *
 * @brief Typed thread-safe queue interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <memory>

namespace Async
{
  template<class T>
  class Queue
  {
  public:
    using Ptr = std::shared_ptr<Queue<T>>;

    virtual ~Queue() = default;

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
}  // namespace Async
