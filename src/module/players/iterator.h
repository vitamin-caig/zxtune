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

// library includes
#include <module/state.h>

namespace Module
{
  struct LoopParameters;

  class Iterator
  {
  public:
    typedef std::shared_ptr<Iterator> Ptr;

    virtual ~Iterator() = default;

    virtual void Reset() = 0;
    virtual bool IsValid() const = 0;
    virtual void NextFrame(const LoopParameters& looped) = 0;
  };

  class StateIterator : public Iterator
  {
  public:
    typedef std::shared_ptr<StateIterator> Ptr;

    virtual uint_t CurrentFrame() const = 0;

    virtual State::Ptr GetStateObserver() const = 0;
  };

  void SeekIterator(Iterator& iter, State::Ptr state, Time::AtMillisecond pos);
}  // namespace Module
