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

#include "apps/zxtune-qt/parameters.h"

namespace Parameters::ZXTuneQT::UI
{
  constexpr auto Prefix = ZXTuneQT::Prefix + "UI"_id;

  //! @brief Interface language
  struct Language : Value<StringType>
  {
    static constexpr auto Key = Prefix + "Language"_id;
  };

  // Per-widget state sub-namespace suffixes; the namespace is dynamic, so these are keys, not descriptors
  const auto PARAM_GEOMETRY = "Geometry"sv;
  const auto PARAM_LAYOUT = "Layout"sv;
  const auto PARAM_VISIBLE = "Visible"sv;
  const auto PARAM_INDEX = "Index"sv;
  const auto PARAM_SIZE = "Size"sv;
}  // namespace Parameters::ZXTuneQT::UI
