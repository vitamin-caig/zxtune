/**
 *
 * @file
 *
 * @brief  IO objects identifiers template support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <strings/template.h>

#include <string_view.h>

namespace IO
{
  Strings::Template::Ptr CreateFilenameTemplate(StringView notation);
}
