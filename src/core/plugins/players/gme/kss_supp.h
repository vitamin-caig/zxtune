/**
 *
 * @file
 *
 * @brief  KSS format support tools
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <string_view.h>
// library includes
#include <binary/view.h>

namespace Module::KSS
{
  StringView DetectPlatform(Binary::View data);
}
