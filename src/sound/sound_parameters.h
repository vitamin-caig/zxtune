/*
Abstract:
  Render parameters names

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/
#ifndef __SOUND_PARAMETERS_H_DEFINED__
#define __SOUND_PARAMETERS_H_DEFINED__

#include <types.h>

namespace ZXTune
{
  namespace Parameters
  {
    namespace Sound
    {
      //rendering-related parameters
      //! Hz, int64_t
      const int64_t FREQUENCY_DEFAULT = 44100;
      const Char FREQUENCY[] = {'s', 'o', 'u', 'n', 'd', '.', 'f', 'r', 'e', 'q', 'u', 'e', 'n', 'c', 'y', '\0'};
      //! Hz, int64_t
      const int64_t CLOCKRATE_DEFAULT = 1750000;
      const Char CLOCKRATE[] = {'s', 'o', 'u', 'n', 'd', '.', 'c', 'l', 'o', 'c', 'k', 'r', 'a', 't', 'e', '\0'};
      //! uS, int64_t
      const int64_t FRAMEDURATION_DEFAULT = 20000;
      const Char FRAMEDURATION[] = {'s', 'o', 'u', 'n', 'd', '.', 'f', 'r', 'a', 'm', 'e', 'd', 'u', 'r', 'a', 't', 'i', 'o', 'n', 0};
    }
  }
}

#endif //__SOUND_PARAMETERS_H_DEFINED__
