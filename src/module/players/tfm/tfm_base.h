/**
 *
 * @file
 *
 * @brief  TFM-based chiptunes support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/players/tfm/tfm_chiptune.h"

#include "module/holder.h"

namespace Module::TFM
{
  Holder::Ptr CreateHolder(Chiptune::Ptr chiptune, Parameters::Accessor::Ptr properties);
}  // namespace Module::TFM
