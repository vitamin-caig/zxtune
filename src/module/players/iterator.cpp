/**
* 
* @file
*
* @brief  State iterators support
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "iterator.h"

namespace Module
{
  void SeekIterator(StateIterator& iter, uint_t frameNum)
  {
    const TrackState::Ptr state = iter.GetStateObserver();
    uint_t curFrame = state->Frame();
    if (curFrame > frameNum)
    {
      iter.Reset();
      curFrame = 0;
    }
    while (curFrame < frameNum && iter.IsValid())
    {
      iter.NextFrame(true);
      ++curFrame;
    }
  }
}
