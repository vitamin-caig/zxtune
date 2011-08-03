/*
Abstract:
  State iterator functions implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "iterator.h"

namespace ZXTune
{
  namespace Module
  {
    void SeekIterator(StateIterator& iter, uint_t frameNum)
    {
      const TrackState::Ptr state = iter.GetStateObserver();
      if (state->Frame() > frameNum)
      {
        iter.Reset();
      }
      while (state->Frame() < frameNum && iter.IsValid())
      {
        iter.NextFrame(false);
      }
    }
  }
}
