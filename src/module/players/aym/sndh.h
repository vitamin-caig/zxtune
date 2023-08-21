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

// library includes
#include <module/players/factory_multitrack.h>

namespace Module::SNDH
{
  MultitrackFactory::Ptr CreateFactory();
}  // namespace Module::SNDH
