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
#include "ui/parameters.h"

namespace Parameters
{
  namespace ZXTuneQT
  {
    namespace UI
    {
      namespace Export
      {
        const auto PREFIX = UI::PREFIX + "Export"_id;
        const auto NAMESPACE_NAME = static_cast<Identifier>(PREFIX).Name();

        const auto TYPE = PREFIX + "Type"_id;
      }  // namespace Export
    }    // namespace UI
  }      // namespace ZXTuneQT
}  // namespace Parameters
