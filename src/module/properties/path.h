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

// library includes
#include <io/identifier.h>
#include <parameters/accessor.h>

namespace Module
{
  Parameters::Accessor::Ptr CreatePathProperties(StringView fullpath);
  Parameters::Accessor::Ptr CreatePathProperties(IO::Identifier::Ptr id);
}  // namespace Module
