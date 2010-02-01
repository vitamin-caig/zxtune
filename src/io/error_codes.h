/**
*
* @file      io/error_codes.h
* @brief     IO subsystem error codes
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#ifndef __IO_ERROR_CODES_H_DEFINED__
#define __IO_ERROR_CODES_H_DEFINED__

#include <error.h>

namespace ZXTune
{
  //! @brief %IO functions and types namespace
  namespace IO
  {
    //! @brief IO-related error codes
    enum ErrorCode
    {
      //! Specified access scheme is not supported
      NOT_SUPPORTED = Error::ModuleCode<'I', 'O', 'S'>::Value,
      //! Failt to open data container
      NOT_OPENED,
      //! Specified file is not found
      NOT_FOUND,
      //! Data retrieving is canceled
      CANCELED,
      //! Access to file is denied by OS
      NO_ACCESS,
      //! %IO error happends while opening the file
      IO_ERROR,
      //! File already exists
      FILE_EXISTS
    };
  }
}

#endif //__IO_ERROR_CODES_H_DEFINED__
