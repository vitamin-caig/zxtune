/**
 *
 * @file
 *
 * @brief  TFD chiptune factory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/players/tfm/tfm_factory.h"

namespace Module::TFD
{
  TFM::Factory::Ptr CreateFactory();
}  // namespace Module::TFD
