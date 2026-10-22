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

#include "apps/zxtune-qt/ui/parameters.h"

namespace Parameters::ZXTuneQT::UI::Export
{
  constexpr auto Prefix = UI::Prefix + "Export"_id;
  const auto NAMESPACE_NAME = static_cast<Identifier>(Prefix).Name();

  //! @brief Export target type
  struct Type : Value<StringType>
  {
    static constexpr auto Key = Prefix + "Type"_id;
  };
}  // namespace Parameters::ZXTuneQT::UI::Export
