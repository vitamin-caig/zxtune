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
      if (iter.Frame() > frameNum)
      {
        iter.Reset();
      }
      while (iter.Frame() < frameNum)
      {
        if (!iter.NextFrame(false))
        {
          break;
        }
      }
    }
  }
}
