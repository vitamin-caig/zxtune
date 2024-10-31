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

#include <io/identifier.h>
#include <parameters/accessor.h>

#include <string_view.h>

namespace Module
{
  Parameters::Accessor::Ptr CreatePathProperties(StringView fullpath);
  Parameters::Accessor::Ptr CreatePathProperties(IO::Identifier::Ptr id);
}  // namespace Module
