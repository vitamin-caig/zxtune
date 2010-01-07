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

#include <parameters.h>

namespace Parameters
{
  namespace ZXTune
  {
    namespace Sound
    {
      //per-backend parameters
      namespace Backends
      {
        //! template, String
        const Char FILENAME[] =
        {
          'z','x','t','u','n','e','.','s','o','u','n','d','.','b','a','c','k','e','n','d','s','.','f','i','l','e','n','a','m','e','\0'
        };

        //parameters for Win32 backend
        namespace Win32
        {
          const IntType DEVICE_DEFAULT = -1;
          const char DEVICE[] =
          {
            'z','x','t','u','n','e','.','s','o','u','n','d','.','b','a','c','k','e','n','d','s','.','w','i','n','3','2','.','d','e','v','i','c','e','\0'
          };
        }

        //parameters for OSS backend
        namespace OSS
        {
          const Char DEVICE_DEFAULT[] = {'/', 'd', 'e', 'v', '/', 'd', 's', 'p', '\0'};
          const Char DEVICE[] =
          {
            'z','x','t','u','n','e','.','s','o','u','n','d','.','b','a','c','k','e','n','d','s','.','o','s','s','.','d','e','v','i','c','e','\0'
          };
          const Char MIXER_DEFAULT[] = {'/', 'd', 'e', 'v', '/', 'm', 'i', 'x', 'e', 'r', '\0'};
          const Char MIXER[] =
          {
            'z','x','t','u','n','e','.','s','o','u','n','d','.','b','a','c','k','e','n','d','s','.','o','s','s','.','m','i','x','e','r','\0'
          };
        }
      }
    }
  }
}
#endif //__SOUND_BACKENDS_PARAMETERS_H_DEFINED__
