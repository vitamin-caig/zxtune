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

  const std::string PARAM_GEOMETRY("Geometry");
  const std::string PARAM_LAYOUT("Layout");
  const std::string PARAM_VISIBLE("Visible");
  const std::string PARAM_INDEX("Index");
  const std::string PARAM_SIZE("Size");
}  // namespace Parameters::ZXTuneQT::UI
