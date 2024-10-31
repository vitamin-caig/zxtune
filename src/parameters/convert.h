/**
 *
 * @file
 *
 * @brief  Parameters conversion functions
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <parameters/types.h>

#include <string_view.h>

#include <optional>

namespace Parameters
{
  //! @brief Converting parameter value to string
  String ConvertToString(IntType val);
  String ConvertToString(StringView val);
  String ConvertToString(Binary::View val);

  //! @brief Converting parameter value from string
  std::optional<IntType> ConvertIntegerFromString(StringView str);
  std::optional<StringType> ConvertStringFromString(StringView str);
  Binary::Data::Ptr ConvertDataFromString(StringView str);
}  // namespace Parameters
