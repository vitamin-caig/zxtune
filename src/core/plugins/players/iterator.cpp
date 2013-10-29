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
