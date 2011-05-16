/*
Abstract:
  State iterator interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __CORE_PLUGINS_PLAYERS_STATE_ITERATOR_H_DEFINED__
#define __CORE_PLUGINS_PLAYERS_STATE_ITERATOR_H_DEFINED__

//library includes
#include <core/module_types.h>
#include <sound/render_params.h>// for LoopMode

namespace ZXTune
{
  namespace Module
  {
    class StateIterator : public TrackState
    {
    public:
      typedef boost::shared_ptr<StateIterator> Ptr;

      virtual void Reset() = 0;

      virtual void ResetPosition() = 0;

      virtual bool NextFrame(uint64_t ticksToSkip, Sound::LoopMode mode) = 0;
    };
  }
}

#endif //__CORE_PLUGINS_PLAYERS_STATE_ITERATOR_H_DEFINED__
