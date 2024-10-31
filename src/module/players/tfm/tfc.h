/**
 *
 * @file
 *
 * @brief  TFC chiptune factory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/players/tfm/tfm_factory.h"

namespace Module::TFC
{
  TFM::Factory::Ptr CreateFactory();
}  // namespace Module::TFC
