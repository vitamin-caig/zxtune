/*
Abstract:
  State iterator functions implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "state_iterator.h"

namespace ZXTune
{
  namespace Module
  {
    void SeekIterator(StateIterator& iter, uint_t frameNum)
    {
      assert(iter.Frame() <= frameNum);
      while (iter.Frame() < frameNum)
      {
        if (!iter.NextFrame(0, Sound::LOOP_NONE))
        {
          break;
        }
      }
    }
  }
}
