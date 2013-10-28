/**
*
* @file     parameters/convert.h
* @brief    Parameters conversion functions
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef PARAMETERS_CONVERT_H_DEFINED
#define PARAMETERS_CONVERT_H_DEFINED

//library includes
#include <parameters/types.h>

namespace Parameters
{
  //! @brief Converting parameter value to string
  String ConvertToString(const IntType& val);
  String ConvertToString(const StringType& val);
  String ConvertToString(const DataType& val);

  //! @brief Converting parameter value from string
  bool ConvertFromString(const String& str, IntType& res);
  bool ConvertFromString(const String& str, StringType& res);
  bool ConvertFromString(const String& str, DataType& res);
}

#endif //PARAMETERS_CONVERT_H_DEFINED
