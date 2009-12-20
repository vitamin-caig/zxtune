/*
Abstract:
  Backends parameters names

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/
#ifndef __SOUND_BACKENDS_PARAMETERS_H_DEFINED__
#define __SOUND_BACKENDS_PARAMETERS_H_DEFINED__

#include <char_type.h>

namespace ZXTune
{
  namespace Parameters
  {
    namespace Sound
    {
      //per-backend parameters
      namespace Backends
      {
        //! template, String
        const Char FILENAME[] = {'s', 'o', 'u', 'n', 'd', '.', 'b', 'a', 'c', 'k', 'e', 'n', 'd', 's', '.', 'f', 'i', 'l', 'e', 'n', 'a', 'm', 'e', 0};
      }
    }
  }
}
#endif //__SOUND_BACKENDS_PARAMETERS_H_DEFINED__
