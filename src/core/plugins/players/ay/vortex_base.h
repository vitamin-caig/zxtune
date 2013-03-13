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

      //SimpleOrnament::Loop is not used
      typedef SimpleOrnament Ornament;

      class ModuleData : public TrackModel
      {
      public:
        typedef boost::shared_ptr<ModuleData> RWPtr;
        typedef boost::shared_ptr<const ModuleData> Ptr;

        ModuleData()
          : InitialTempo()
        {
        }

        virtual uint_t GetInitialTempo() const
        {
          return InitialTempo;
        }

        virtual const OrderList& GetOrder() const
        {
          return *Order;
        }

        virtual const PatternsSet& GetPatterns() const
        {
          return *Patterns;
        }

        uint_t InitialTempo;
        OrderList::Ptr Order;
        PatternsSet::Ptr Patterns;
        SparsedObjectsStorage<Sample> Samples;
        SparsedObjectsStorage<Ornament> Ornaments;
      };

      uint_t ExtractVersion(const Parameters::Accessor& props);

      //creating simple player based on parsed data and parameters
      Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, ModuleData::Ptr data,
         uint_t version, Devices::AYM::Chip::Ptr device, uint_t trackChannelStart = 0);

      AYM::Chiptune::Ptr CreateChiptune(ModuleData::Ptr data, ModuleProperties::Ptr properties);
    }
  }
}

#endif //__CORE_PLUGINS_PLAYERS_VORTEX_BASE_H_DEFINED__
