/**
*
* @file
*
* @brief  Core parameters names
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <parameters/types.h>

namespace Parameters
{
  namespace ZXTune
  {
    //! @brief Core parameters namespace
    namespace Core
    {
      //! @brief Parameters#ZXTune#Core namespace prefix
      extern const NameType PREFIX;

      //! @brief AYM-chip related parameters namespace
      namespace AYM
      {
        //! @brief Parameters#ZXTune#Core#AYM namespace prefix
        extern const NameType PREFIX;

        //@{
        //! @name PSG clockrate in Hz

        //! Default value- 1.75MHz
        const IntType CLOCKRATE_DEFAULT = 1750000;
        const IntType CLOCKRATE_MIN = 1000000;
        const IntType CLOCKRATE_MAX = UINT64_C(10000000);
        //! Parameter name
        extern const NameType CLOCKRATE;
        //@}

        //! @brief Chip type
        //! @details 0 is AY, else is YM
        extern const NameType TYPE;

        //! @brief Use interpolation
        //! @details integer value
        extern const NameType INTERPOLATION;

        //! @brief Frequency table for ay-based plugins
        //! @details String- table name or dump @see freq_tables.h
        extern const NameType TABLE;

        //! @brief Duty cycle in percents
        //! @details Integer. Valid values are 1..99. Default is 50
        extern const NameType DUTY_CYCLE;

        //! @brief Duty cycle applied channels masks
        //! @details @see core/devices/aym.h
        extern const NameType DUTY_CYCLE_MASK;

        //! @brief Channels layout parameter
        //! @details @see core/devices/aym.h
        extern const NameType LAYOUT;
      }

      //! @brief DAC-related parameters namespace
      namespace DAC
      {
        //! @brief Parameters#ZXTune#Core#DAC namespace prefix
        extern const NameType PREFIX;

        //! @brief Use interpolation
        //! @details Integer value
        extern const NameType INTERPOLATION;

        const IntType SAMPLES_FREQUENCY_MIN = 800;
        const IntType SAMPLES_FREQUENCY_MAX = 16000;
        //! @brief Base samples frequency for C-1 (32.7Hz)
        extern const NameType SAMPLES_FREQUENCY;
      }

      //! @brief Z80-related parameters namespace
      namespace Z80
      {
        //! @brief Parameters#ZXTune#Core#Z80 namespace prefix
        extern const NameType PREFIX;

        //@{
        //! @name CPU int duration in ticks

        //! Default value
        const IntType INT_TICKS_DEFAULT = 24;
        //! Parameter name
        extern const NameType INT_TICKS;
        //@}

          //@{
        //! @name CPU clockrate in Hz

        //! Default value- 3.5MHz
        const IntType CLOCKRATE_DEFAULT = UINT64_C(3500000);
        const IntType CLOCKRATE_MIN = 1000000;
        const IntType CLOCKRATE_MAX = UINT64_C(10000000);
        //! Parameter name
        extern const NameType CLOCKRATE;
        //@}
      }

      //! @brief FM-related parameters namespace
      namespace FM
      {
        //! @brief Parameters#ZXTune#Core#FM namespace prefix
        extern const NameType PREFIX;

        //@{
        //! @name FM clockrate in Hz

        //! Default value- 3.5MHz
        const IntType CLOCKRATE_DEFAULT = UINT64_C(3500000);
        //! Parameter name
        extern const NameType CLOCKRATE;
        //@}
      }

      //! @brief SAA-related parameters namespace
      namespace SAA
      {
        //! @brief Parameters#ZXTune#Core#SAA namespace prefix
        extern const NameType PREFIX;

        //@{
        //! @name SAA clockrate in Hz

        //! Default value- 8MHz
        const IntType CLOCKRATE_DEFAULT = UINT64_C(8000000);
        const IntType CLOCKRATE_MIN = UINT64_C(4000000);
        const IntType CLOCKRATE_MAX = UINT64_C(16000000);
        //! Parameter name
        extern const NameType CLOCKRATE;
        //@}

        //! @brief Use interpolation
        //! @details integer value
        extern const NameType INTERPOLATION;
      }

      //! @brief SID-related parameters namespace
      namespace SID
      {
        //! @brief Parameters#ZXTune#Core#SID namespace prefix
        extern const NameType PREFIX;

        namespace ROM
        {
          //! @brief Parameters#ZXTune#Core#SID#ROM namespace prefix
          extern const NameType PREFIX;
          //@{
          //! @name ROMs content
          //! 8192 bytes
          extern const NameType KERNAL;
          //! 8192 bytes
          extern const NameType BASIC;
          //! 4096 bytes
          extern const NameType CHARGEN;
        }
      }
    }
  }
}
