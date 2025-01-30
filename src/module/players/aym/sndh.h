/**
 *
 * @file
 *
 * @brief  SNDH chiptune factory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/players/factory_multitrack.h"

namespace Module::SNDH
{
  MultitrackFactory::Ptr CreateFactory();
}  // namespace Module::SNDH
