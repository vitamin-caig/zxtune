/**
* 
* @file
*
* @brief  MIDI-based chiptunes support
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "midi_chiptune.h"
#include "core/plugins/players/renderer.h"
//library includes
#include <sound/render_params.h>

namespace Module
{
  namespace MIDI
  {
    Analyzer::Ptr CreateAnalyzer(Devices::MIDI::Device::Ptr device);

    Renderer::Ptr CreateRenderer(Sound::RenderParameters::Ptr params, DataIterator::Ptr iterator, Devices::MIDI::Device::Ptr device);
  }
}
