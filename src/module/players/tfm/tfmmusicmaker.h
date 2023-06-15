/**
 *
 * @file
 *
 * @brief  TFMMusicMaker chiptune factory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// local includes
#include "module/players/tfm/tfm_factory.h"
// library includes
#include <formats/chiptune/fm/tfmmusicmaker.h>

namespace Module::TFMMusicMaker
{
  TFM::Factory::Ptr CreateFactory(Formats::Chiptune::TFMMusicMaker::Decoder::Ptr decoder);
}  // namespace Module::TFMMusicMaker
