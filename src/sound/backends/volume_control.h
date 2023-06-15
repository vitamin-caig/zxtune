/**
 *
 * @file
 *
 * @brief  Volume control delegate factory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// library includes
#include <sound/backend.h>

namespace Sound
{
  VolumeControl::Ptr CreateVolumeControlDelegate(const VolumeControl::Ptr& delegate);
}
