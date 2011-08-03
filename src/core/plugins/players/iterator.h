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
      virtual bool IsValid() const = 0;
      virtual void NextFrame(bool looped) = 0;
    };

    class StateIterator : public Iterator
    {
    public:
      typedef boost::shared_ptr<StateIterator> Ptr;

      virtual TrackState::Ptr GetStateObserver() const = 0;
    };

    void SeekIterator(StateIterator& iter, uint_t frameNum);
  }
}

#endif //__CORE_PLUGINS_PLAYERS_ITERATOR_H_DEFINED__
