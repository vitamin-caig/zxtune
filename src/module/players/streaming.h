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

namespace Module
{
  Information::Ptr CreateStreamInfo(uint_t frames, uint_t loopFrame = 0);

  StateIterator::Ptr CreateStreamStateIterator(Information::Ptr info);
}
