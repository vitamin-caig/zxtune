/**
*
* @file
*
* @brief  String optimization
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//common includes
#include <types.h>

namespace Strings
{
  //trim all the non-ascii symbols from begin/end and replace by '?' at the middle
  String Optimize(const String& str);
}
