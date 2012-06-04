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
        //@{
        //! @name PSG clockrate in Hz

        //! Default value- 1.75MHz
        const IntType CLOCKRATE_DEFAULT = 1750000;
        //! Parameter name
        const Char CLOCKRATE[] =
        {
          'z','x','t','u','n','e','.','c','o','r','e','.','a','y','m','.','c','l','o','c','k','r','a','t','e','\0'
        };
        //@}

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

      //! @brief DAC-related parameters namespace
      namespace DAC
      {
        //! @brief Use interpolation
        //! @details Integer value
        const Char INTERPOLATION[] =
        {
          'z','x','t','u','n','e','.','c','o','r','e','.','d','a','c','.','i','n','t','e','r','p','o','l','a','t','i','o','n','\0'
        };
      }

      //! @brief Z80-related parameters namespace
      namespace Z80
      {
        //@{
        //! @name CPU int duration in ticks

        //! Default value
        const IntType INT_TICKS_DEFAULT = 24;
        //! Parameter name
        const Char INT_TICKS[] =
        {
          'z','x','t','u','n','e','.','c','o','r','e','.','z','8','0','.','i','n','t','_','t','i','c','k','s','\0'
        };
        //@}

          //@{
        //! @name CPU clockrate in Hz

        //! Default value- 3.5MHz
        const IntType CLOCKRATE_DEFAULT = 3500000;
        //! Parameter name
        const Char CLOCKRATE[] =
        {
          'z','x','t','u','n','e','.','c','o','r','e','.','z','8','0','.','c','l','o','c','k','r','a','t','e','\0'
        };
        //@}
      }

      //! @brief FM-related parameters namespace
      namespace FM
      {
        //@{
        //! @name FM clockrate in Hz

        //! Default value- 3.5MHz
        const IntType CLOCKRATE_DEFAULT = 3500000;
        //! Parameter name
        const Char CLOCKRATE[] =
        {
          'z','x','t','u','n','e','.','c','o','r','e','.','f','m','.','c','l','o','c','k','r','a','t','e','\0'
        };
        //@}
      }
    }
  }
}

#endif //__CORE_PARAMETERS_H_DEFINED__
