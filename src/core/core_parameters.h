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

//local includes
#include <zxtune.h>

namespace Parameters
{
  namespace ZXTune
  {
    //! @brief Core parameters namespace
    namespace Core
    {
      //! @brief Parameters#ZXTune#Core namespace prefix
      const NameType PREFIX = ZXTune::PREFIX + "core";

      //! @brief AYM-chip related parameters namespace
      namespace AYM
      {
        //! @brief Parameters#ZXTune#Core#AYM namespace prefix
        const NameType PREFIX = Core::PREFIX + "aym";

        //@{
        //! @name PSG clockrate in Hz

        //! Default value- 1.75MHz
        const IntType CLOCKRATE_DEFAULT = 1750000;
        const IntType CLOCKRATE_MIN = 1000000;
        const IntType CLOCKRATE_MAX = 10000000;
        //! Parameter name
        const NameType CLOCKRATE = PREFIX + "clockrate";
        //@}

        //! @brief Chip type
        //! @details 0 is AY, else is YM
        const NameType TYPE = PREFIX + "type";

        //! @brief Use interpolation
        //! @details integer value
        const NameType INTERPOLATION = PREFIX + "interpolation";

        //! @brief Frequency table for ay-based plugins
        //! @details String- table name or dump @see freq_tables.h
        const NameType TABLE = PREFIX + "table";

        //! @brief Duty cycle in percents
        //! @details Integer. Valid values are 1..99. Default is 50
        const NameType DUTY_CYCLE = PREFIX + "duty_cycle";

        //! @brief Duty cycle applied channels masks
        //! @details @see core/devices/aym.h
        const NameType DUTY_CYCLE_MASK = PREFIX + "duty_cycle_mask";

        //! @brief Channels layout parameter
        //! @details @see core/devices/aym.h
        const NameType LAYOUT = PREFIX + "layout";
      }

      //! @brief DAC-related parameters namespace
      namespace DAC
      {
        //! @brief Parameters#ZXTune#Core#DAC namespace prefix
        const NameType PREFIX = Core::PREFIX + "dac";

        //! @brief Use interpolation
        //! @details Integer value
        const NameType INTERPOLATION = PREFIX + "interpolation";
      }

      //! @brief Z80-related parameters namespace
      namespace Z80
      {
        //! @brief Parameters#ZXTune#Core#Z80 namespace prefix
        const NameType PREFIX = Core::PREFIX + "z80";

        //@{
        //! @name CPU int duration in ticks

        //! Default value
        const IntType INT_TICKS_DEFAULT = 24;
        //! Parameter name
        const NameType INT_TICKS = PREFIX + "int_ticks";
        //@}

          //@{
        //! @name CPU clockrate in Hz

        //! Default value- 3.5MHz
        const IntType CLOCKRATE_DEFAULT = 3500000;
        const IntType CLOCKRATE_MIN = 1000000;
        const IntType CLOCKRATE_MAX = 10000000;
        //! Parameter name
        const NameType CLOCKRATE = PREFIX + "clockrate";
        //@}
      }

      //! @brief FM-related parameters namespace
      namespace FM
      {
        //! @brief Parameters#ZXTune#Core#FM namespace prefix
        const NameType PREFIX = Core::PREFIX + "fm";

        //@{
        //! @name FM clockrate in Hz

        //! Default value- 3.5MHz
        const IntType CLOCKRATE_DEFAULT = 3500000;
        //! Parameter name
        const NameType CLOCKRATE = PREFIX + "clockrate";
        //@}
      }
    }
  }
}

#endif //__CORE_PARAMETERS_H_DEFINED__
