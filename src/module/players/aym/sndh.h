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

#include "module/players/factory.h"

namespace Module::SNDH
{
  MultitrackFactory::Ptr CreateFactory();
}  // namespace Module::SNDH
