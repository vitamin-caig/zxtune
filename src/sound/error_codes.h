/**
*
* @file      sound/error_codes.h
* @brief     Sound subsystem error codes definitions
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#ifndef __SOUND_ERROR_CODES_H_DEFINED__
#define __SOUND_ERROR_CODES_H_DEFINED__

#include <error.h>

namespace ZXTune
{
  //! @brief %Sound functions and types namespace
  namespace Sound
  {
    //! @brief Sound-related error codes
    enum ErrorCode
    {
      //! Specified mixer input channels count is not supported
      MIXER_UNSUPPORTED = Error::ModuleCode<'M', 'I', 'X'>::Value,
      //! Input mixer parameters are invalid
      MIXER_INVALID_PARAMETER,
      //! Input filter parameters are invalid
      FILTER_INVALID_PARAMETER = Error::ModuleCode<'F', 'L', 'T'>::Value,
      //! %Backend with specified id is not supported
      BACKEND_NOT_FOUND = Error::ModuleCode<'B', 'N', 'D'>::Value,
      //! Failed to creating backend
      BACKEND_FAILED_CREATE,
      //! Invalid parameter while backend parameters setting up
      BACKEND_INVALID_PARAMETER,
      //! Failed to perform complex backend setup
      BACKEND_SETUP_ERROR,
      //! Failed to perform complex backend action
      BACKEND_CONTROL_ERROR,
      //! Platform-specific error
      BACKEND_PLATFORM_ERROR,
      //! Function is not supported
      BACKEND_UNSUPPORTED_FUNC
    };
  }
}

#endif //__SOUND_ERROR_CODES_H_DEFINED__
