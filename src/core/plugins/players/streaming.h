/*
Abstract:
  Streamed modules support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __CORE_PLUGINS_PLAYERS_STREAMING_H_DEFINED__
#define __CORE_PLUGINS_PLAYERS_STREAMING_H_DEFINED__

//local includes
#include "iterator.h"

namespace ZXTune
{
  namespace Module
  {
    Information::Ptr CreateStreamInfo(uint_t frames, uint_t loopFrame = 0);

    StateIterator::Ptr CreateStreamStateIterator(Information::Ptr info);
  }
}

#endif //__CORE_PLUGINS_PLAYERS_STREAMING_H_DEFINED__
