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

#include <formats/chiptune/digital/abysshighestexperience.h>
#include <module/players/factory.h>

namespace Module::AHX
{
  Factory::Ptr CreateFactory(Formats::Chiptune::AbyssHighestExperience::Decoder::Ptr decoder);
}  // namespace Module::AHX
