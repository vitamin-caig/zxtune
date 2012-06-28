/*
Abstract:
  ALSA subsystem access functions

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef SOUND_BACKENDS_ALSA_H_DEFINED
#define SOUND_BACKENDS_ALSA_H_DEFINED

//common includes
#include <string_helpers.h>

namespace ZXTune
{
  namespace Sound
  {
    namespace ALSA
    {
      StringArray EnumerateDevices();
      StringArray EnumerateMixers(const String& device);
    }
  }
}

#endif //SOUND_BACKENDS_ALSA_H_DEFINED
