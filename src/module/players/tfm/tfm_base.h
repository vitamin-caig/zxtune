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

//local includes
#include "module/players/tfm/tfm_chiptune.h"
//library includes
#include <module/renderer.h>
#include <sound/render_params.h>

namespace Module
{
  namespace TFM
  {
    Analyzer::Ptr CreateAnalyzer(Devices::TFM::Device::Ptr device);

    Renderer::Ptr CreateRenderer(Sound::RenderParameters::Ptr params, DataIterator::Ptr iterator, Devices::TFM::Device::Ptr device);
  }
}
