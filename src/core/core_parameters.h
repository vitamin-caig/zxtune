/**
*
* @file     core/core_parameters.h
* @brief    Core parameters names
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef __CORE_PARAMETERS_H_DEFINED__
#define __CORE_PARAMETERS_H_DEFINED__

//common includes
#include <parameters.h>

namespace Parameters
{
  namespace ZXTune
  {
    //! @brief Core parameters namespace
    namespace Core
    {
      //! @brief Parameters#ZXTune#Core namespace prefix
      const Char PREFIX[] =
      {
        'z','x','t','u','n','e','.','c','o','r','e','.','\0'
      };

      //! @brief AYM-chip related parameters namespace
      namespace AYM
      {
        //! @brief Chip type
        //! @details 0 is AY, else is YM
        const Char TYPE[] =
        {
          'z','x','t','u','n','e','.','c','o','r','e','.','a','y','m','.','t','y','p','e','\0'
        };
        //! @brief Use interpolation
        //! @details integer value
        const Char INTERPOLATION[] =
        {
          'z','x','t','u','n','e','.','c','o','r','e','.','a','y','m','.','i','n','t','e','r','p','o','l','a','t','i','o','n','\0'
        };
        //! @brief Frequency table for ay-based plugins
        //! @details String- table name or dump @see freq_tables.h
        const Char TABLE[] =
        {
          'z','x','t','u','n','e','.','c','o','r','e','.','a','y','m','.','t','a','b','l','e','\0'
        };
        //! @brief Duty cycle in percents
        //! @details Integer. Valid values are 1..99. Default is 50
        const Char DUTY_CYCLE[] =
        {
          'z','x','t','u','n','e','.','c','o','r','e','.','a','y','m','.','d','u','t','y','_','c','y','c','l','e','\0'
        };
        //! @brief Duty cycle applied channels masks
        //! @details @see core/devices/aym.h
        const Char DUTY_CYCLE_MASK[] =
        {
          'z','x','t','u','n','e','.','c','o','r','e','.','a','y','m','.','d','u','t','y','_','c','y','c','l','e','_','m','a','s','k','\0'
        };
        //! @brief Channels layout parameter
        //! @details @see core/devices/aym.h
        const Char LAYOUT[] =
        {
          'z','x','t','u','n','e','.','c','o','r','e','.','a','y','m','.','l','a','y','o','u','t','\0'
        };
      }

      //! @brief DAC-realated parameters namespace
      namespace DAC
      {
        //! @brief Use interpolation
        //! @details Integer value
        const Char INTERPOLATION[] =
        {
          'z','x','t','u','n','e','.','c','o','r','e','.','d','a','c','.','i','n','t','e','r','p','o','l','a','t','i','o','n','\0'
        };
      }
    }
  }
}

#endif //__CORE_PARAMETERS_H_DEFINED__
