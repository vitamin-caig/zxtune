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
    uint_t curFrame = iter.GetStateObserver()->Frame();
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
