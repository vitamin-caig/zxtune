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

// common includes
#include <string_view.h>
// library includes
#include <strings/template.h>

namespace IO
{
  Strings::Template::Ptr CreateFilenameTemplate(StringView notation);
}
