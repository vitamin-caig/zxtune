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
        //! @brief Parameters#ZXTune#IO#Providers namespace prefix
        extern const NameType PREFIX;

        //! @brief %File provider parameters namespace
        namespace File
        {
          //! @brief Parameters#ZXTune#IO#Providers#File namespace prefix
          extern const NameType PREFIX;
          //@{
          //! @name Memory-mapping usage data size threshold parameter.

          //! Default value
          const IntType MMAP_THRESHOLD_DEFAULT = 16384;
          //! Parameter full path
          extern const NameType MMAP_THRESHOLD;
          //@}

          //@{
          //! @name Create intermediate directories

          //! Default value
          const IntType CREATE_DIRECTORIES_DEFAULT = 1;
          //! @Parameter full path
          extern const NameType CREATE_DIRECTORIES;
          //@}

          //@{
          //! @name Overwrite files

          //! Default value
          const IntType OVERWRITE_EXISTING_DEFAULT = 0;
          //! @Parameter full path
          extern const NameType OVERWRITE_EXISTING;
          //@}
        }

        //! @brief %Network provider parameters namespace
        namespace Network
        {
          //! @brief Parameters#ZXTune#IO#Providers#Network namespace prefix
          extern const NameType PREFIX;

          //! @brief Parameters for HTTP protocol
          namespace Http
          {
            //! @brief Parameters#ZXTune#IO#Providers#Network#Http namespace prefix
            extern const NameType PREFIX;

            //@{
            //! @name Useragent parameter

            //! Parameter full path
            extern const NameType USERAGENT;
          }
        }
      }
    }
  }
}
#endif //__IO_PROVIDERS_PARAMETERS_H_DEFINED__
