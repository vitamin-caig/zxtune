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

#include "parameters/value.h"

namespace Parameters::ZXTune::Core
{
  constexpr auto Prefix = ZXTune::PREFIX + "core"_id;

  //! @brief Masking for each of ATTR_CHANNELS_NAMES channels
  struct ChannelsMask : Value<uint64_t>
  {
    static constexpr auto Key = Prefix + "channels_mask"_id;
    static constexpr Type Default = 0;
  };

  //! @brief AYM-chip related parameters namespace
  namespace AYM
  {
    constexpr auto Prefix = Core::Prefix + "aym"_id;

    //@{
    //! @name PSG clockrate in Hz
    struct Clockrate : Value<uint_t>
    {
      static constexpr auto Key = Prefix + "clockrate"_id;
      static constexpr Type Default = 1'750'000, Min = 100'000, Max = 10'000'000;
    };
    //@}

    //! @details Type is Devices::AYM::ChipType
    struct Chip : Value<uint_t>
    {
      static constexpr auto Key = Prefix + "type"_id;
      static constexpr Type Default = 0;
    };

    //! @details Type is Devices::AYM::InterpolationType
    struct Interpolation : Value<uint_t>
    {
      static constexpr auto Key = Prefix + "interpolation"_id;
      static constexpr Type Default = 2;
    };

    //! @brief Duty cycle in percents
    struct DutyCycle : Value<uint_t>
    {
      static constexpr auto Key = Prefix + "duty_cycle"_id;
      static constexpr Type Default = 50, Min = 1, Max = 99;
    };

    //! @brief Frequency table for ay-based plugins: name or dump (@see freq_tables.h)
    struct Table : Value<StringType>
    {
      static constexpr auto Key = Prefix + "table"_id;
    };

    //! @brief Duty cycle applied channels mask: number or letters (@see devices/aym.h)
    struct DutyCycleMask : Value<uint_t>
    {
      static constexpr auto Key = Prefix + "duty_cycle_mask"_id;
      static constexpr Type Default = 0;
    };

    //! @brief Channels layout (@see devices/aym/chip.h)
    //! @details Type is Devices::AYM::LayoutType
    struct Layout : Value<uint_t>
    {
      static constexpr auto Key = Prefix + "layout"_id;
      static constexpr Type Default = 0;
    };
  }  // namespace AYM

  //! @brief DAC-related parameters namespace
  namespace DAC
  {
    constexpr auto Prefix = Core::Prefix + "dac"_id;

    //! @brief Base samples frequency for C-1 (32.7Hz)
    struct SamplesFrequency : Value<uint_t>
    {
      static constexpr auto Key = Prefix + "samples_frequency"_id;
      static constexpr Type Min = 800, Max = 16'000;
    };

    //! @brief Use interpolation for DAC rendering
    struct Interpolation : Value<bool>
    {
      static constexpr auto Key = Prefix + "interpolation"_id;
      static constexpr Type Default = false;
    };
  }  // namespace DAC

  //! @brief Z80-related parameters namespace
  namespace Z80
  {
    constexpr auto Prefix = Core::Prefix + "z80"_id;

    //@{
    //! @name CPU clockrate in Hz
    struct Clockrate : Value<uint_t>
    {
      static constexpr auto Key = Prefix + "clockrate"_id;
      static constexpr Type Default = 3'500'000, Min = 1'000'000, Max = 10'000'000;
    };
    //@}

    //! @brief CPU int duration in ticks
    struct IntTicks : Value<uint_t>
    {
      static constexpr auto Key = Prefix + "int_ticks"_id;
      static constexpr Type Default = 24, Min = 1, Max = 100;
    };
  }  // namespace Z80

  //! @brief FM-related parameters namespace
  namespace FM
  {
    constexpr auto Prefix = Core::Prefix + "fm"_id;

    //! @brief FM clockrate in Hz
    struct Clockrate : Value<uint_t>
    {
      static constexpr auto Key = Prefix + "clockrate"_id;
      static constexpr Type Default = 3'500'000, Min = 1'000'000, Max = 10'000'000;
    };
  }  // namespace FM

  //! @brief SAA-related parameters namespace
  namespace SAA
  {
    constexpr auto Prefix = Core::Prefix + "saa"_id;

    //@{
    //! @name SAA clockrate in Hz
    struct Clockrate : Value<uint_t>
    {
      static constexpr auto Key = Prefix + "clockrate"_id;
      static constexpr Type Default = 8'000'000, Min = 4'000'000, Max = 16'000'000;
    };
    //@}

    //! @details Type is Devices::SAA::InterpolationType
    struct Interpolation : Value<uint_t>
    {
      static constexpr auto Key = Prefix + "interpolation"_id;
      static constexpr Type Default = 1;
    };
  }  // namespace SAA

  //! @brief SID-related parameters namespace
  namespace SID
  {
    constexpr auto Prefix = Core::Prefix + "sid"_id;

    //! @brief Use SID filter emulation
    struct Filter : Value<bool>
    {
      static constexpr auto Key = Prefix + "filter"_id;
      static constexpr Type Default = true;
    };

    //! @details 0 = none, 1 = low, 2 = high
    struct Interpolation : Value<uint_t>
    {
      static constexpr auto Key = Prefix + "interpolation"_id;
      static constexpr Type Default = 0;
    };
  }  // namespace SID
}  // namespace Parameters::ZXTune::Core
