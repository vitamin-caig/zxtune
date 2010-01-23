/**
*
* @file      io/io_parameters.h
* @brief     IO parameters names
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#ifndef __IO_PARAMETERS_H_DEFINED__
#define __IO_PARAMETERS_H_DEFINED__

#include <parameters.h>

namespace Parameters
{
  namespace ZXTune
  {
    //! @brief IO-parameters namespace
    namespace IO
    {
      //! @brief Parameters#ZXTune#IO namespace prefix
      const Char PREFIX[] = 
      {
        'z','x','t','u','n','e','.','i','o','.','\0'
      };
      //IO-related parameters
    }
  }
}

#endif //__SOUND_PARAMETERS_H_DEFINED__
