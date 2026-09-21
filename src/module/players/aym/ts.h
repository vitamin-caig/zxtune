/**
 *
 * @file
 *
 * @brief  TurboSound-based chiptunes support factory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/players/factory.h"

namespace Module::TS
{
  Factory::Ptr CreateFactory(Factory::Ptr delegate);
}  // namespace Module::TS
