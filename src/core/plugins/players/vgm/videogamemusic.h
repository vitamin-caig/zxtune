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

#include <binary/view.h>

#include <string_view.h>

namespace Module::VideoGameMusic
{
  StringView DetectPlatform(Binary::View data);
}  // namespace Module::VideoGameMusic
