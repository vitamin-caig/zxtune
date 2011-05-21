/*
Abstract:
  Module terator interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __CORE_PLUGINS_PLAYERS_ITERATOR_H_DEFINED__
#define __CORE_PLUGINS_PLAYERS_ITERATOR_H_DEFINED__

//library includes
#include <core/module_types.h>

namespace ZXTune
{
  namespace Module
  {
    class Iterator
    {
    public:
      typedef boost::shared_ptr<Iterator> Ptr;

      virtual ~Iterator() {}

      virtual void Reset() = 0;

      virtual bool NextFrame(uint64_t ticksToSkip, bool looped) = 0;

      virtual void Seek(uint_t frameNum) = 0;
    };

    class StateIterator : public TrackState
                        , public Iterator
    {
    public:
      typedef boost::shared_ptr<StateIterator> Ptr;
    };

    //! @invariant iter.Frame() <= frameNum
    void SeekIterator(StateIterator& iter, uint_t frameNum);
  }
}

#endif //__CORE_PLUGINS_PLAYERS_ITERATOR_H_DEFINED__
