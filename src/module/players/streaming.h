/**
* 
* @file
*
* @brief  Streamed modules support
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "module/players/iterator.h"
//library includes
#include <module/information.h>
#include <time/duration.h>

namespace Module
{
  struct FramedStream
  {
    uint_t TotalFrames = 0;
    uint_t LoopFrame = 0;
    Time::Microseconds FrameDuration;
  };

  Information::Ptr CreateStreamInfo(FramedStream stream);

  StateIterator::Ptr CreateStreamStateIterator(FramedStream stream);
}
