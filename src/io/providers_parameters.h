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

//local includes
#include "io_parameters.h"

namespace Parameters
{
  namespace ZXTune
  {
    namespace IO
    {
      //! @brief Providers-related %IO parameters namespace
      namespace Providers
      {
        //! @brief Parameters#ZXTune#IO#Providers namespace prefix
        const NameType PREFIX = IO::PREFIX + "providers";

        //! @brief %File provider parameters namespace
        namespace File
        {
          //! @brief Parameters#ZXTune#IO#Providers#File namespace prefix
          const NameType PREFIX = Providers::PREFIX + "file";
          //@{
          //! @name Memory-mapping usage data size threshold parameter.

          //! Default value
          const IntType MMAP_THRESHOLD_DEFAULT = 16384;
          //! Parameter full path
          const NameType MMAP_THRESHOLD = PREFIX + "mmap_threshold";
          //@}
        }

        //! @brief %Network provider parameters namespace
        namespace Network
        {
          //! @brief Parameters#ZXTune#IO#Providers#Network namespace prefix
          const NameType PREFIX = Providers::PREFIX + "network";

          //! @brief Parameters for HTTP protocol
          namespace Http
          {
            //! @brief Parameters#ZXTune#IO#Providers#Network#Http namespace prefix
            const NameType PREFIX = Network::PREFIX + "http";

            //@{
            //! @name Useragent parameter

            //! Parameter full path
            const NameType USERAGENT = PREFIX + "useragent";
          }
        }
      }
    }
  }
}
#endif //__IO_PROVIDERS_PARAMETERS_H_DEFINED__
