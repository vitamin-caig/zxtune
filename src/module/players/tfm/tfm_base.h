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

namespace Module
{
  namespace TFM
  {
    Analyzer::Ptr CreateAnalyzer(Devices::TFM::Device::Ptr device);

    Renderer::Ptr CreateRenderer(const Parameters::Accessor& params, DataIterator::Ptr iterator, Devices::TFM::Device::Ptr device);
  }
}
