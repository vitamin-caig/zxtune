/*
Abstract:
  Vortex-based modules playback support interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef __CORE_PLUGINS_PLAYERS_VORTEX_BASE_H_DEFINED__
#define __CORE_PLUGINS_PLAYERS_VORTEX_BASE_H_DEFINED__

#include "tracking.h"

#include <core/module_holder.h>
#include <core/devices/aym.h>

namespace ZXTune
{
  namespace Module
  {
    struct VortexSample
    {
      VortexSample() : Loop(), Data()
      {
      }

      VortexSample(unsigned size, unsigned loop) : Loop(loop), Data(size)
      {
      }

      //make safe sample
      void Fix()
      {
        if (Data.empty())
        {
          Data.resize(1);
        }
      }

      struct Line
      {
        Line()
         : Level(), VolumeSlideAddon(), ToneMask(true), ToneOffset(), KeepToneOffset()
         , NoiseMask(true), EnvMask(true), NoiseOrEnvelopeOffset(), KeepNoiseOrEnvelopeOffset()
        {
        }

        // level-related
        unsigned Level;//0-15
        signed VolumeSlideAddon;
        // tone-related
        bool ToneMask;
        signed ToneOffset;
        bool KeepToneOffset;
        // noise/enelope-related
        bool NoiseMask;
        bool EnvMask;
        signed NoiseOrEnvelopeOffset;
        bool KeepNoiseOrEnvelopeOffset;
      };

      unsigned Loop;
      std::vector<Line> Data;
    };
    
    //commands set
    enum VortexCommands
    {
      EMPTY,
      GLISS,        //2p: period,delta
      GLISS_NOTE,   //3p: period,delta,note
      SAMPLEOFFSET, //1p: offset
      ORNAMENTOFFSET,//1p:offset
      VIBRATE,      //2p: yestime,notime
      SLIDEENV,     //2p: period,delta
      NOENVELOPE,   //0p
      ENVELOPE,     //2p: type, base
      NOISEBASE,    //1p: base
      TEMPO,        //1p - pseudo-effect
    };

    typedef TrackingSupport<AYM::CHANNELS, VortexSample> VortexTrack;

    Player::Ptr CreateVortexPlayer(Holder::ConstPtr holder, const VortexTrack::ModuleData& data, 
       unsigned version, const String& freqTableName, AYM::Chip::Ptr device);
  }
}

#endif //__CORE_PLUGINS_PLAYERS_VORTEX_BASE_H_DEFINED__
