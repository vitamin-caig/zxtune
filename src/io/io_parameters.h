/**
*
* @file      io/io_parameters.h
* @brief     IO parameters names
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef __IO_PARAMETERS_H_DEFINED__
#define __IO_PARAMETERS_H_DEFINED__

//local includes
#include <zxtune.h>

namespace Parameters
{
  namespace ZXTune
  {
    //! @brief IO-parameters namespace
    namespace IO
    {
      //! @brief Parameters#ZXTune#IO namespace prefix
      const NameType PREFIX = ZXTune::PREFIX + "io";
      //IO-related parameters
    }
  }
}

#endif //__SOUND_PARAMETERS_H_DEFINED__
