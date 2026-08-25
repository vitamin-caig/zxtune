/**
 *
 * @file
 *
 * @brief  libvgm-based formats support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/players/factory.h"

namespace Module::VideoGameMusic
{
  Factory::Ptr CreateFactory();
}  // namespace Module::VideoGameMusic

namespace Module::Sound98
{
  Factory::Ptr CreateFactory();
}  // namespace Module::Sound98
