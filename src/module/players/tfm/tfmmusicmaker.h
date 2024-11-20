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

#include "formats/chiptune/fm/tfmmusicmaker.h"
#include "module/players/tfm/tfm_factory.h"

namespace Module::TFMMusicMaker
{
  TFM::Factory::Ptr CreateFactory(Formats::Chiptune::TFMMusicMaker::Decoder::Ptr decoder);
}  // namespace Module::TFMMusicMaker
