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

//library includes
#include <module/track_state.h>

namespace Module
{
  class Iterator
  {
  public:
    typedef std::shared_ptr<Iterator> Ptr;

    virtual ~Iterator() {}

    virtual void Reset() = 0;
    virtual bool IsValid() const = 0;
    virtual void NextFrame(bool looped) = 0;
  };

  class StateIterator : public Iterator
  {
  public:
    typedef std::shared_ptr<StateIterator> Ptr;

    virtual TrackState::Ptr GetStateObserver() const = 0;
  };

  void SeekIterator(StateIterator& iter, uint_t frameNum);
}
