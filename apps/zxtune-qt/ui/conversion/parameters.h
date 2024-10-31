/**
 *
 * @file
 *
 * @brief Export parameters declaration
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// local includes
#include "apps/zxtune-qt/ui/parameters.h"

namespace Parameters::ZXTuneQT::UI::Export
{
  const auto PREFIX = UI::PREFIX + "Export"_id;
  const auto NAMESPACE_NAME = static_cast<Identifier>(PREFIX).Name();

  const auto TYPE = PREFIX + "Type"_id;
}  // namespace Parameters::ZXTuneQT::UI::Export
