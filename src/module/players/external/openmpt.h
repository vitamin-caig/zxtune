/**
 *
 * @file
 *
 * @brief  libopenmpt-based formats support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/players/factory.h"

#include "string_view.h"

namespace Module::MPT
{
  ExternalParsingFactory::Ptr CreateFactory(StringView id);
}  // namespace Module::MPT
