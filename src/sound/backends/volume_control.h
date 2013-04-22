/*
Abstract:
  Volume control delegate factory

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef SOUND_BACKENDS_VOLUME_CONTROL_H_DEFINED
#define SOUND_BACKENDS_VOLUME_CONTROL_H_DEFINED

//library includes
#include <sound/backend.h>

namespace ZXTune
{
  namespace Sound
  {
    VolumeControl::Ptr CreateVolumeControlDelegate(VolumeControl::Ptr delegate);
  }
}

#endif //SOUND_BACKENDS_VOLUME_CONTROL_H_DEFINED
