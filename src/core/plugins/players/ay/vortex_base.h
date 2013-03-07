/*
Abstract:
  Vortex-based modules playback support interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __CORE_PLUGINS_PLAYERS_VORTEX_BASE_H_DEFINED__
#define __CORE_PLUGINS_PLAYERS_VORTEX_BASE_H_DEFINED__

//local includes
#include "ay_base.h"
//library includes
#include <core/module_holder.h>
#include <devices/aym.h>
#include <formats/chiptune/protracker3.h>

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
      struct Sample : public Formats::Chiptune::ProTracker3::Sample
      {
        Sample() : Formats::Chiptune::ProTracker3::Sample()
        {
        }

        Sample(const Formats::Chiptune::ProTracker3::Sample& rh)
          : Formats::Chiptune::ProTracker3::Sample(rh)
        {
        }

        template<class Iterator>
        Sample(uint_t loop, Iterator from, Iterator to)
        {
          Loop = loop;
          Lines.assign(from, to);
        }

        uint_t GetLoop() const
        {
          return Loop;
        }

        uint_t GetSize() const
        {
          return static_cast<uint_t>(Lines.size());
        }

        const Line& GetLine(const uint_t idx) const
        {
          static const Line STUB;
          return Lines.size() > idx ? Lines[idx] : STUB;
        }
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
      };

      typedef TrackingSupport<Devices::AYM::CHANNELS, Sample> Track;

      uint_t ExtractVersion(const Parameters::Accessor& props);

      //creating simple player based on parsed data and parameters
      Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Track::ModuleData::Ptr data,
         uint_t version, Devices::AYM::Chip::Ptr device);

      AYM::Chiptune::Ptr CreateChiptune(Track::ModuleData::Ptr data, ModuleProperties::Ptr properties, uint_t channels);
    }
  }
}

#endif //__CORE_PLUGINS_PLAYERS_VORTEX_BASE_H_DEFINED__
