/**
 *
 * @file
 *
 * @brief  ASAP-based formats support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/players/factory.h"

#include "string_view.h"

namespace Module::ASAP
{
  MultitrackFactory::Ptr CreateMultitrackFactory(StringView id);
  ExternalParsingFactory::Ptr CreateFactory(StringView id);
}  // namespace Module::ASAP
