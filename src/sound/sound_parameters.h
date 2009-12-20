/*
Abstract:
  Render parameters attributes

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/
#ifndef __SOUND_ATTRS_H_DEFINED__
#define __SOUND_ATTRS_H_DEFINED__

namespace ZXTune
{
  namespace Sound
  {
    namespace Parameters
    {
      //rendering-related parameters
      //! Hz, int64_t
      const Char FREQUENCY[] = {'s', 'o', 'u', 'n', 'd', '.', 'f', 'r', 'e', 'q', 'u', 'e', 'n', 'c', 'y', '\0'};
      //! Hz, int64_t
      const Char CLOCKRATE[] = {'s', 'o', 'u', 'n', 'd', '.', 'c', 'l', 'o', 'c', 'k', 'r', 'a', 't', 'e', '\0'};
      //! uS, int64_t
      const Char FRAMEDURATION[] = {'s', 'o', 'u', 'n', 'd', '.', 'f', 'r', 'a', 'm', 'e', 'd', 'u', 'r', 'a', 't', 'i', 'o', 'n', 0};
    }
  }
}

#endif //__SOUND_ATTRS_H_DEFINED__
