/**
 *
 * @file
 *
 * @brief UI parameters definition
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// local includes
#include "app_parameters.h"

namespace Parameters::ZXTuneQT::UI
{
  const auto PREFIX = ZXTuneQT::PREFIX + "UI"_id;

  const auto LANGUAGE = PREFIX + "Language"_id;

  const auto PARAM_GEOMETRY = "Geometry"sv;
  const auto PARAM_LAYOUT = "Layout"sv;
  const auto PARAM_VISIBLE = "Visible"sv;
  const auto PARAM_INDEX = "Index"sv;
  const auto PARAM_SIZE = "Size"sv;
}  // namespace Parameters::ZXTuneQT::UI
