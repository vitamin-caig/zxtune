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

// library includes
#include <parameters/identifier.h>
#include <parameters/types.h>

namespace Parameters
{
  namespace ZXTuneQT
  {
    const auto PREFIX = "zxtune-qt"_id;

    const auto SINGLE_INSTANCE = PREFIX + "SingleInstance"_id;
    const IntType SINGLE_INSTANCE_DEFAULT = 0;
  }  // namespace ZXTuneQT
}  // namespace Parameters
