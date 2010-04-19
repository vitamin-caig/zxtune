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

//common includes
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
      ERROR_NOT_SUPPORTED = Error::ModuleCode<'I', 'O', 'S'>::Value,
      //! Failt to open data container
      ERROR_NOT_OPENED,
      //! Specified file is not found
      ERROR_NOT_FOUND,
      //! Data retrieving is canceled
      ERROR_CANCELED,
      //! Access to file is denied by OS
      ERROR_NO_ACCESS,
      //! %IO error happends while opening the file
      ERROR_IO_ERROR,
      //! File already exists
      ERROR_FILE_EXISTS,
      //! Failed to allocate memory
      ERROR_NO_MEMORY
    };
  }
}

#endif //__IO_ERROR_CODES_H_DEFINED__
