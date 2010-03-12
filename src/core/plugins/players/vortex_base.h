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
    namespace Vortex
    {
      enum NoteTable
      {
        PROTRACKER,
        SOUNDTRACKER,
        ASM,
        REAL
      };

      struct Sample
      {
        Sample() : Loop(), Data()
        {
        }

        Sample(uint_t size, uint_t loop) : Loop(loop), Data(size)
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
          uint_t Level;//0-15
          int_t VolumeSlideAddon;
          // tone-related
          bool ToneMask;
          int_t ToneOffset;
          bool KeepToneOffset;
          // noise/enelope-related
          bool NoiseMask;
          bool EnvMask;
          int_t NoiseOrEnvelopeOffset;
          bool KeepNoiseOrEnvelopeOffset;
        };

        uint_t Loop;
        std::vector<Line> Data;
      };

      //commands set
      enum Commands
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

      //for unification
      typedef SimpleOrnament Ornament;

      typedef TrackingSupport<AYM::CHANNELS, Sample, Ornament> Track;

      Player::Ptr CreatePlayer(Holder::ConstPtr holder, const Track::ModuleData& data,
         uint_t version, const String& freqTableName, AYM::Chip::Ptr device);
    }
  }
}

#endif //__CORE_PLUGINS_PLAYERS_VORTEX_BASE_H_DEFINED__
