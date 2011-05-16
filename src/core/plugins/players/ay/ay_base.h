/*
Abstract:
  AYM-based modules support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __CORE_PLUGINS_PLAYERS_AY_BASE_H_DEFINED__
#define __CORE_PLUGINS_PLAYERS_AY_BASE_H_DEFINED__

//local includes
#include "aym_parameters_helper.h"
#include <core/plugins/players/streaming.h>
#include <core/plugins/players/tracking.h>
//common includes
#include <error.h>
//library includes
#include <devices/aym.h>
#include <sound/render_params.h>

namespace ZXTune
{
  namespace Module
  {
    typedef PatternCursorSet<AYM::CHANNELS> AYMPatternCursors;

    class AYMDataRenderer
    {
    public:
      typedef boost::shared_ptr<AYMDataRenderer> Ptr;

      virtual ~AYMDataRenderer() {}

      virtual void SynthesizeData(const TrackState& state, AYMTrackSynthesizer& synthesizer) = 0;
      virtual void Reset() = 0;
    };

    Renderer::Ptr CreateAYMRenderer(AYM::ParametersHelper::Ptr params, StateIterator::Ptr iterator, AYMDataRenderer::Ptr renderer, AYM::Chip::Ptr device);

    Renderer::Ptr CreateAYMStreamRenderer(Information::Ptr info, AYMDataRenderer::Ptr renderer, AYM::Chip::Ptr device);

    Renderer::Ptr CreateAYMTrackRenderer(Information::Ptr info, TrackModuleData::Ptr data,
      AYMDataRenderer::Ptr renderer, AYM::Chip::Ptr device, const String& defaultTable);
  }
}

#endif //__CORE_PLUGINS_PLAYERS_AY_BASE_H_DEFINED__
