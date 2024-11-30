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

#include "zxtune.h"

#include "parameters/types.h"

namespace Parameters::ZXTune::Core
{
  //! @brief Parameters#ZXTune#Core namespace prefix
  const auto PREFIX = ZXTune::PREFIX + "core"_id;

  //! @brief AYM-chip related parameters namespace
  namespace AYM
  {
    //! @brief Parameters#ZXTune#Core#AYM namespace prefix
    const auto PREFIX = Core::PREFIX + "aym"_id;

    //@{
    //! @name PSG clockrate in Hz

    //! Default value- 1.75MHz
    const IntType CLOCKRATE_DEFAULT = 1750000;
    const IntType CLOCKRATE_MIN = 100000;
    const IntType CLOCKRATE_MAX = UINT64_C(10000000);
    //! Parameter name
    const auto CLOCKRATE = PREFIX + "clockrate"_id;
    //@}

    //@{
    //! @name Chip type
    const IntType TYPE_AY = 0;
    const IntType TYPE_YM = 1;
    //! Default is AY
    const IntType TYPE_DEFAULT = TYPE_AY;
    //! Parameter name
    const auto TYPE = PREFIX + "type"_id;
    //@}

    //@{
    //! @name Interpolation mode
    const IntType INTERPOLATION_NONE = 0;
    const IntType INTERPOLATION_LQ = 1;
    const IntType INTERPOLATION_HQ = 2;
    //! Default is HQ
    const IntType INTERPOLATION_DEFAULT = INTERPOLATION_HQ;
    //! Parameter name
    const auto INTERPOLATION = PREFIX + "interpolation"_id;
    //@}

    //! @brief Frequency table for ay-based plugins
    //! @details String- table name or dump @see freq_tables.h
    const auto TABLE = PREFIX + "table"_id;

    //@{
    //! @name Duty cycle in percents
    const IntType DUTY_CYCLE_MIN = 1;
    const IntType DUTY_CYCLE_MAX = 99;
    // Default is 50%
    const IntType DUTY_CYCLE_DEFAULT = 50;
    //! Parameter name
    const auto DUTY_CYCLE = PREFIX + "duty_cycle"_id;
    //@}

    //@{
    //! @name Duty cycle applied channels masks
    //! @details @see core/devices/aym.h
    const IntType DUTY_CYCLE_MASK_DEFAULT = 0;
    //! Parameter name
    const auto DUTY_CYCLE_MASK = PREFIX + "duty_cycle_mask"_id;
    //@}

    //@{
    //! @name Channels layout parameter
    //! @details @see core/devices/aym/chip.h
    const IntType LAYOUT_ABC = 0;
    const IntType LAYOUT_ACB = 1;
    const IntType LAYOUT_BAC = 2;
    const IntType LAYOUT_BCA = 3;
    const IntType LAYOUT_CAB = 4;
    const IntType LAYOUT_CBA = 5;
    const IntType LAYOUT_MONO = 6;
    //! Default is ABC
    const IntType LAYOUT_DEFAULT = LAYOUT_ABC;
    //! Parameter name
    const auto LAYOUT = PREFIX + "layout"_id;
    //@}
  }  // namespace AYM

  //! @brief DAC-related parameters namespace
  namespace DAC
  {
    //! @brief Parameters#ZXTune#Core#DAC namespace prefix
    const auto PREFIX = Core::PREFIX + "dac"_id;

    //@{
    //! @name Interpolation mode
    const IntType INTERPOLATION_NO = 0;
    const IntType INTERPOLATION_YES = 1;
    //! Default is LQ
    const IntType INTERPOLATION_DEFAULT = INTERPOLATION_NO;
    //! Parameter name
    const auto INTERPOLATION = PREFIX + "interpolation"_id;
    //@}

    const IntType SAMPLES_FREQUENCY_MIN = 800;
    const IntType SAMPLES_FREQUENCY_MAX = 16000;
    //! @brief Base samples frequency for C-1 (32.7Hz)
    const auto SAMPLES_FREQUENCY = PREFIX + "samples_frequency"_id;
  }  // namespace DAC

  //! @brief Z80-related parameters namespace
  namespace Z80
  {
    //! @brief Parameters#ZXTune#Core#Z80 namespace prefix
    const auto PREFIX = Core::PREFIX + "z80"_id;

    //@{
    //! @name CPU int duration in ticks

    //! Default value
    const IntType INT_TICKS_DEFAULT = 24;
    //! Parameter name
    const auto INT_TICKS = PREFIX + "int_ticks"_id;
    //@}

    //@{
    //! @name CPU clockrate in Hz

    //! Default value- 3.5MHz
    const IntType CLOCKRATE_DEFAULT = UINT64_C(3500000);
    const IntType CLOCKRATE_MIN = 1000000;
    const IntType CLOCKRATE_MAX = UINT64_C(10000000);
    //! Parameter name
    const auto CLOCKRATE = PREFIX + "clockrate"_id;
    //@}
  }  // namespace Z80

  //! @brief FM-related parameters namespace
  namespace FM
  {
    //! @brief Parameters#ZXTune#Core#FM namespace prefix
    const auto PREFIX = Core::PREFIX + "fm"_id;

    //@{
    //! @name FM clockrate in Hz

    //! Default value- 3.5MHz
    const IntType CLOCKRATE_DEFAULT = UINT64_C(3500000);
    //! Parameter name
    const auto CLOCKRATE = PREFIX + "clockrate"_id;
    //@}
  }  // namespace FM

  //! @brief SAA-related parameters namespace
  namespace SAA
  {
    //! @brief Parameters#ZXTune#Core#SAA namespace prefix
    const auto PREFIX = Core::PREFIX + "saa"_id;

    //@{
    //! @name SAA clockrate in Hz

    //! Default value- 8MHz
    const IntType CLOCKRATE_DEFAULT = UINT64_C(8000000);
    const IntType CLOCKRATE_MIN = UINT64_C(4000000);
    const IntType CLOCKRATE_MAX = UINT64_C(16000000);
    //! Parameter name
    const auto CLOCKRATE = PREFIX + "clockrate"_id;
    //@}

    //@{
    //! @name Interpolation mode
    const IntType INTERPOLATION_NONE = 0;
    const IntType INTERPOLATION_LQ = 1;
    const IntType INTERPOLATION_HQ = 2;
    //! Default is LQ
    const IntType INTERPOLATION_DEFAULT = INTERPOLATION_LQ;
    //! Parameter name
    const auto INTERPOLATION = PREFIX + "interpolation"_id;
    //@}
  }  // namespace SAA

  //! @brief SID-related parameters namespace
  namespace SID
  {
    //! @brief Parameters#ZXTune#Core#SID namespace prefix
    const auto PREFIX = Core::PREFIX + "sid"_id;

    //@{
    //! @name SID filter emulation
    const IntType FILTER_DISABLED = 0;
    const IntType FILTER_ENABLED = 1;
    //! Default is enabled
    const IntType FILTER_DEFAULT = FILTER_ENABLED;
    //! Parameter name
    const auto FILTER = PREFIX + "filter"_id;
    //@}

    //@{
    //! @name Interpolation mode
    const IntType INTERPOLATION_NONE = 0;
    const IntType INTERPOLATION_LQ = 1;
    const IntType INTERPOLATION_HQ = 2;
    //! Default is LQ
    const IntType INTERPOLATION_DEFAULT = INTERPOLATION_NONE;
    //! Parameter name
    const auto INTERPOLATION = PREFIX + "interpolation"_id;
    //@}
  }  // namespace SID
}  // namespace Parameters::ZXTune::Core
