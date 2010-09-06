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

//local includes
#include "../tracking.h"
//library includes
#include <core/module_holder.h>
#include <devices/aym.h>

namespace ZXTune
{
  namespace Module
  {
    //at least two formats are based on Vortex, so it's useful to extract tracking-related types
    namespace Vortex
    {
      // Frequency table enumeration, compatible with binary format (PT3.x)
      enum NoteTable
      {
        PROTRACKER,
        SOUNDTRACKER,
        ASM,
        REAL,
        NATURAL
      };
      
      String GetFreqTable(NoteTable table, uint_t version);

      //sample type
      struct Sample
      {
        Sample() : Loop(), Lines()
        {
        }

        template<class Iterator>
        Sample(uint_t loop, Iterator from, Iterator to)
          : Loop(loop), Lines(from, to)
        {
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

        uint_t GetLoop() const
        {
          return Loop;
        }

        uint_t GetSize() const
        {
          return Lines.size();
        }

        const Line& GetLine(const uint_t idx) const
        {
          static const Line STUB;
          return Lines.size() > idx ? Lines[idx] : STUB;
        }
      private:
        uint_t Loop;
        std::vector<Line> Lines;
      };

      //supported commands set and their parameters
      enum Commands
      {
        //no parameters
        EMPTY,
        //period,delta
        GLISS,
        //period,delta,target note
        GLISS_NOTE,
        //offset
        SAMPLEOFFSET,
        //offset
        ORNAMENTOFFSET,
        //ontime,offtime
        VIBRATE,
        //period,delta
        SLIDEENV,
        //no parameters
        NOENVELOPE,
        //r13,period
        ENVELOPE,
        //base
        NOISEBASE,
        //tempo
        TEMPO
      };

      //for unification
      typedef SimpleOrnament Ornament;

      typedef TrackingSupport<AYM::CHANNELS, Sample, Ornament> Track;

      //creating simple player based on parsed data and parameters
      Player::Ptr CreatePlayer(Information::Ptr info, Track::ModuleData::Ptr data,
         uint_t version, const String& freqTableName, AYM::Chip::Ptr device);
      //creating TS player based on parsed data and parameters
      Player::Ptr CreateTSPlayer(Information::Ptr info, Track::ModuleData::Ptr data,
         uint_t version, const String& freqTableName, uint_t patternBase, AYM::Chip::Ptr device1, AYM::Chip::Ptr device2);
    }
  }
}

#endif //__CORE_PLUGINS_PLAYERS_VORTEX_BASE_H_DEFINED__
