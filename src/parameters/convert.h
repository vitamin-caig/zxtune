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

// library includes
#include <parameters/types.h>

namespace Parameters
{
  //! @brief Converting parameter value to string
  String ConvertToString(IntType val);
  String ConvertToString(StringView val);
  String ConvertToString(Binary::View val);

  //! @brief Converting parameter value from string
  bool ConvertFromString(StringView str, IntType& res);
  bool ConvertFromString(StringView str, StringType& res);
  Binary::Data::Ptr ConvertDataFromString(StringView str);
}  // namespace Parameters
