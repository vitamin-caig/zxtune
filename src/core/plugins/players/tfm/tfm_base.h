/*
Abstract:
  TFM-based players common functionality

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef CORE_PLUGINS_PLAYERS_TFM_BASE_DEFINED
#define CORE_PLUGINS_PLAYERS_TFM_BASE_DEFINED

//local includes
#include "tfm_chiptune.h"
#include "core/plugins/players/renderer.h"
//library includes
#include <sound/render_params.h>

namespace Module
{
  namespace TFM
  {
    Analyzer::Ptr CreateAnalyzer(Devices::TFM::Device::Ptr device);

    Renderer::Ptr CreateRenderer(Sound::RenderParameters::Ptr params, DataIterator::Ptr iterator, Devices::TFM::Device::Ptr device);
  }
}

#endif //CORE_PLUGINS_PLAYERS_TFM_BASE_DEFINED
