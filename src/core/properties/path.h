/**
* 
* @file
*
* @brief  Path properties support
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <core/module_holder.h>
#include <io/identifier.h>

namespace Module
{
  Parameters::Accessor::Ptr CreatePathProperties(const String& fullpath);
  Parameters::Accessor::Ptr CreatePathProperties(IO::Identifier::Ptr id);
}
