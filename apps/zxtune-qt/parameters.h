/**
 *
 * @file
 *
 * @brief Application parameters definitions
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "parameters/value.h"

namespace Parameters::ZXTuneQT
{
  constexpr auto Prefix = "zxtune-qt"_id;

  //! @brief Single instance mode
  struct SingleInstance : Value<bool>
  {
    static constexpr auto Key = Prefix + "SingleInstance"_id;
    static constexpr Type Default = false;
  };
}  // namespace Parameters::ZXTuneQT
