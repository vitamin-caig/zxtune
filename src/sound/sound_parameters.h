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

#include <parameters.h>

namespace Parameters
{
  namespace ZXTune
  {
    namespace Sound
    {
      //rendering-related parameters
      //! Hz
      const IntType FREQUENCY_DEFAULT = 44100;
      const Char FREQUENCY[] = 
      {
        'z','x','t','u','n','e','.','s','o','u','n','d','.','f','r','e','q','u','e','n','c','y','\0'
      };
      //! Hz
      const IntType CLOCKRATE_DEFAULT = 1750000;
      const Char CLOCKRATE[] = 
      {
        'z','x','t','u','n','e','.','s','o','u','n','d','.','c','l','o','c','k','r','a','t','e','\0'
      };
      //! uS
      const IntType FRAMEDURATION_DEFAULT = 20000;
      const Char FRAMEDURATION[] = 
      {
        'z','x','t','u','n','e','.','s','o','u','n','d','.','f','r','a','m','e','d','u','r','a','t','i','o','n','\0'
      };
    }
  }
}

#endif //__SOUND_PARAMETERS_H_DEFINED__
