/*
Abstract:
  AYM-based modules support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

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

    class AYMDevice : public Analyzer
    {
    public:
      typedef boost::shared_ptr<AYMDevice> Ptr;

      //some virtuals from AYM::Chip
      virtual void RenderData(const Sound::RenderParameters& params,
                              const AYM::DataChunk& src,
                              Sound::MultichannelReceiver& dst) = 0;

      virtual void Reset() = 0;

      static Ptr Create(AYM::Chip::Ptr device);
    };

    class AYMDataRenderer
    {
    public:
      typedef boost::shared_ptr<AYMDataRenderer> Ptr;
      
      virtual ~AYMDataRenderer() {}

      virtual void SynthesizeData(const TrackState& state, AYMTrackSynthesizer& synthesizer) = 0;
      virtual void Reset() = 0;
    };

    Player::Ptr CreateAYMStreamPlayer(Information::Ptr info, AYMDataRenderer::Ptr renderer, AYM::Chip::Ptr device);

    Player::Ptr CreateAYMTrackPlayer(Information::Ptr info, TrackModuleData::Ptr data, 
      AYMDataRenderer::Ptr renderer, AYM::Chip::Ptr device, const String& defaultTable);
  }
}

#endif //__CORE_PLUGINS_PLAYERS_AY_BASE_H_DEFINED__
