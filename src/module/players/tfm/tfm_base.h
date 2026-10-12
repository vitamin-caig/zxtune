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

#include "module/players/parameters.h"
#include "module/players/tfm/tfm_chiptune.h"

#include "module/holder.h"
#include "module/renderer.h"

namespace Module::TFM
{
  Renderer::Ptr CreateRenderer(FrameDurationParameter frameDuration, DataIterator::Ptr iterator,
                               Devices::TFM::Chip::Ptr device);

  Holder::Ptr CreateHolder(Chiptune::Ptr chiptune, Parameters::Accessor::Ptr properties);
}  // namespace Module::TFM
