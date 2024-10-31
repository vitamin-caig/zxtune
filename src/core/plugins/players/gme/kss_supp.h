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

#include <binary/view.h>

#include <string_view.h>

namespace Module::KSS
{
  StringView DetectPlatform(Binary::View data);
}
