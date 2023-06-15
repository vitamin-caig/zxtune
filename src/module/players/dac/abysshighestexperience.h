/**
 *
 * @file
 *
 * @brief  AHX chiptune factory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// library includes
#include <module/players/factory.h>
// library includes
#include <formats/chiptune/digital/abysshighestexperience.h>

namespace Module::AHX
{
  Factory::Ptr CreateFactory(Formats::Chiptune::AbyssHighestExperience::Decoder::Ptr decoder);
}  // namespace Module::AHX
