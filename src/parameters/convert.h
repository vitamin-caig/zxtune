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
  String ConvertToString(const IntType& val);
  String ConvertToString(StringView val);
  String ConvertToString(const DataType& val);

  //! @brief Converting parameter value from string
  bool ConvertFromString(const String& str, IntType& res);
  bool ConvertFromString(const String& str, StringType& res);
  bool ConvertFromString(const String& str, DataType& res);
}  // namespace Parameters
