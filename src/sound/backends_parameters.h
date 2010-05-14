/**
*
* @file      sound/backends_parameters.h
* @brief     Backends parameters names
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#ifndef __SOUND_BACKENDS_PARAMETERS_H_DEFINED__
#define __SOUND_BACKENDS_PARAMETERS_H_DEFINED__

//common includes
#include <parameters.h>

namespace Parameters
{
  namespace ZXTune
  {
    namespace Sound
    {
      //! @brief Backend-specific parameters namespace
      namespace Backends
      {
        //! @brief Parameters#ZXTune#Sound#Backends namespace prefix
        const Char PREFIX[] =
        {
          'z','x','t','u','n','e','.','s','o','u','n','d','.','b','a','c','k','e','n','d','s','.','\0'
        };

        //! @brief %Wav backend parameters namespace
        namespace Wav
        {
          //@{
          //! @name Wav backend parameters

          //! @brief Output filename template
          //! @see core/module_attrs.h for possibly supported field names
          const Char FILENAME[] =
          {
            'z','x','t','u','n','e','.','s','o','u','n','d','.','b','a','c','k','e','n','d','s','.','w','a','v','.','f','i','l','e','n','a','m','e','\0'
          };

          //! @brief Rewrite output files if exist
          //! @note != 0 if yes
          const Char OVERWRITE[] =
          {
            'z','x','t','u','n','e','.','s','o','u','n','d','.','b','a','c','k','e','n','d','s','.','w','a','v','.','o','v','e','r','w','r','i','t','e','\0'
          };

          //! @brief Create cuesheet
          //! @note != 0 if yes
          const Char CUESHEET[] =
          {
            'z','x','t','u','n','e','.','s','o','u','n','d','.','b','a','c','k','e','n','d','s','.','w','a','v','.','c','u','e','s','h','e','e','t','\0'
          };

          //@}
        }

        //! @brief %Win32 backend parameters namespace
        namespace Win32
        {
          //@{
          //! @name Win32 backend parameters

          //! Default value
          const IntType DEVICE_DEFAULT = -1;
          //! 0-based output device index
          const Char DEVICE[] =
          {
            'z','x','t','u','n','e','.','s','o','u','n','d','.','b','a','c','k','e','n','d','s','.','w','i','n','3','2','.','d','e','v','i','c','e','\0'
          };

          //! Default value
          const IntType BUFFERS_DEFAULT = 3;
          //! Buffers count
          const Char BUFFERS[] =
          {
            'z','x','t','u','n','e','.','s','o','u','n','d','.','b','a','c','k','e','n','d','s','.','w','i','n','3','2','.','b','u','f','f','e','r','s','\0'
          };
          //@}
        }

        //! @brief %OSS backend parameters
        namespace OSS
        {
          //@{
          //! @name OSS backend parameters

          //! Default value
          const Char DEVICE_DEFAULT[] = {'/', 'd', 'e', 'v', '/', 'd', 's', 'p', '\0'};
          //! Device filename
          const Char DEVICE[] =
          {
            'z','x','t','u','n','e','.','s','o','u','n','d','.','b','a','c','k','e','n','d','s','.','o','s','s','.','d','e','v','i','c','e','\0'
          };

          //! Default value
          const Char MIXER_DEFAULT[] = {'/', 'd', 'e', 'v', '/', 'm', 'i', 'x', 'e', 'r', '\0'};
          //! Mixer filename
          const Char MIXER[] =
          {
            'z','x','t','u','n','e','.','s','o','u','n','d','.','b','a','c','k','e','n','d','s','.','o','s','s','.','m','i','x','e','r','\0'
          };
          //@}
        }

        //! @brief %ALSA backend parameters
        namespace ALSA
        {
          //@{
          //! @name ALSA backend parameters

          //! Default value
          const Char DEVICE_DEFAULT[] = {'d', 'e', 'f', 'a', 'u', 'l', 't', '\0'};
          //! Device name
          const Char DEVICE[] =
          {
            'z','x','t','u','n','e','.','s','o','u','n','d','.','b','a','c','k','e','n','d','s','.','a','l','s','a','.','d','e','v','i','c','e','\0'
          };

          //! Mixer name
          const Char MIXER[] =
          {
            'z','x','t','u','n','e','.','s','o','u','n','d','.','b','a','c','k','e','n','d','s','.','a','l','s','a','.','m','i','x','e','r','\0'
          };

          //! Default value
          const IntType BUFFERS_DEFAULT = 2;
          //! Buffers count
          const Char BUFFERS[] =
          {
            'z','x','t','u','n','e','.','s','o','u','n','d','.','b','a','c','k','e','n','d','s','.','a','l','s','a','.','b','u','f','f','e','r','s','\0'
          };
          //@}
        }

        //! @brief %SDL backend parameters
        namespace SDL
        {
          //@{
          //! @name SDL backend parameters

          //! Default value
          const IntType BUFFERS_DEFAULT = 5;
          //! Buffers count
          const Char BUFFERS[] =
          {
            'z','x','t','u','n','e','.','s','o','u','n','d','.','b','a','c','k','e','n','d','s','.','s','d','l','.','b','u','f','f','e','r','s','\0'
          };
          //@}
        }
      }
    }
  }
}
#endif //__SOUND_BACKENDS_PARAMETERS_H_DEFINED__
