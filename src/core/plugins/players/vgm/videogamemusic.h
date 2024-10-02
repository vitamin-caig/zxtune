/**
 *
 * @file
 *
 * @brief  VGM format support tools
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <string_view.h>
// library includes
#include <binary/view.h>

namespace Module::VideoGameMusic
{
  StringView DetectPlatform(Binary::View data);
}  // namespace Module::VideoGameMusic
