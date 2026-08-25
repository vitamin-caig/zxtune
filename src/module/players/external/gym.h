/**
 *
 * @file
 *
 * @brief  GYM format data decompression support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "binary/container.h"
#include "binary/data.h"

namespace Module::GYM
{
  Binary::Data::Ptr CreateData(const Binary::Container& data);
}  // namespace Module::GYM
