/**
 *
 * @file
 *
 * @brief  Iterators interfaces
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/state.h"

namespace Module
{
  class Iterator
  {
  public:
    using Ptr = std::unique_ptr<Iterator>;

    virtual ~Iterator() = default;

    virtual void Reset() = 0;
    virtual void NextFrame() = 0;
  };

  class StateIterator : public Iterator
  {
  public:
    using Ptr = std::unique_ptr<StateIterator>;

    virtual uint_t CurrentFrame() const = 0;

    virtual State::Ptr GetStateObserver() const = 0;
  };
}  // namespace Module
