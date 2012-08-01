/**
*
* @file      io/providers_parameters.h
* @brief     Providers parameters names
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef __IO_PROVIDERS_PARAMETERS_H_DEFINED__
#define __IO_PROVIDERS_PARAMETERS_H_DEFINED__

//common includes
#include <parameters.h>

namespace Parameters
{
  namespace ZXTune
  {
    namespace IO
    {
      //! @brief Providers-related %IO parameters namespace
      namespace Providers
      {
        //! @brief Parameters#ZXTune#IO#Provders namespace prefix
        const Char PREFIX[] =
        {
          'z','x','t','u','n','e','.','i','o','.','p','r','o','v','i','d','e','r','s','.','\0'
        };

        //! @brief %File provider parameters namespace
        namespace File
        {
          //@{
          //! @name Memory-mapping usage data size threshold parameter.

          //! Default value
          const IntType MMAP_THRESHOLD_DEFAULT = 16384;
          //! Parameter name
          const Char MMAP_THRESHOLD[] =
          {
            'z','x','t','u','n','e','.','i','o','.','p','r','o','v','i','d','e','r','s','.','f','i','l','e','.','m','m','a','p','_','t','h','r','e','s','h','o','l','d','\0'
          };
          //@}
        }

        //! @brief %Network provider parameters namespace
        namespace Network
        {
          //! @brief Parameters for HTTP protocol
          namespace Http
          {
            //@{
            //! @name Useragent parameter

            //! Parameter name
            const Char USERAGENT[] =
            {
              'z','x','t','u','n','e','.','i','o','.','p','r','o','v','i','d','e','r','s','.','n','e','t','w','o','r','k','.','h','t','t','p','.','u','s','e','r','a','g','e','n','t','\0'
            };
          }
        }
      }
    }
  }
}
#endif //__IO_PROVIDERS_PARAMETERS_H_DEFINED__
