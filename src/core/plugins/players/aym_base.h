/*
Abstract:
  AYM-based modules support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef __CORE_PLUGINS_PLAYERS_AYM_BASE_H_DEFINED__
#define __CORE_PLUGINS_PLAYERS_AYM_BASE_H_DEFINED__

//local includes
#include "tracking.h"
#include <core/devices/aym.h>
#include <core/devices/aym_parameters_helper.h>
//common includes
#include <error.h>
//library includes
#include <core/error_codes.h>
#include <core/module_holder.h>
#include <sound/render_params.h>
//text includes
#include <core/text/core.h>

#ifdef FILE_TAG
#error Invalid include sequence
#else
#define FILE_TAG 835AA703
#endif

// perform module 'playback' right after creating (debug purposes)
#ifndef NDEBUG
#define SELF_TEST
#endif

namespace ZXTune
{
  namespace Module
  {
    //common base for all AYM-based players
    class AYMPlayerBase : public Player
    {
    public:
      AYMPlayerBase(AYM::Chip::Ptr device, const String& defTable)
        : Device(device)
        , AYMHelper(AYM::ParametersHelper::Create(defTable))
        , CurrentState(MODULE_STOPPED)
      {
      }

      virtual Error GetPlaybackState(uint_t& timeState,
                                     Tracking& trackState,
                                     Analyze::ChannelsState& analyzeState) const
      {
        timeState = ModState.Frame;
        trackState = ModState.Track;
        Device->GetState(analyzeState);
        return Error();
      }

      virtual Error SetParameters(const Parameters::Map& params)
      {
        try
        {
          AYMHelper->SetParameters(params);
          return Error();
        }
        catch (const Error& e)
        {
          return Error(THIS_LINE, ERROR_INVALID_PARAMETERS, Text::MODULE_ERROR_SET_PLAYER_PARAMETERS).AddSuberror(e);
        }
      }
    protected:
      //aym-related
      AYM::Chip::Ptr Device;
      AYM::ParametersHelper::Ptr AYMHelper;
      //tracking-related
      PlaybackState CurrentState;
      Module::Timing ModState;
    };

    //Common base for all aym-tracking-based players
    //Tracker - instance of TrackingSupport
    //State - internal state type
    template<class Tracker, class State, class ModuleData = typename Tracker::ModuleData>
    class AYMPlayer : public AYMPlayerBase
    {
    public:
      AYMPlayer(typename ModuleData::ConstPtr data,
        AYM::Chip::Ptr device, const String& defTable)
        : AYMPlayerBase(device, defTable)
        , Data(data)
      {
        //WARNING: not a virtual call
        Reset();
      }

      virtual Error RenderFrame(const Sound::RenderParameters& params,
                                PlaybackState& state,
                                Sound::MultichannelReceiver& receiver)
      {

        if (ModState.Frame >= Data->Info.Statistic.Frame)
        {
          if (MODULE_STOPPED == CurrentState)
          {
            return Error(THIS_LINE, ERROR_MODULE_END, Text::MODULE_ERROR_MODULE_END);
          }
          receiver.Flush();
          state = CurrentState = MODULE_STOPPED;
          return Error();
        }

        AYM::DataChunk chunk;
        AYMHelper->GetDataChunk(chunk);
        ModState.Tick += params.ClocksPerFrame();
        chunk.Tick = ModState.Tick;
        RenderData(chunk);

        Device->RenderData(params, chunk, receiver);
        if (Tracker::UpdateState(*Data, ModState, params.Looping))
        {
          CurrentState = MODULE_PLAYING;
        }
        else
        {
          receiver.Flush();
          CurrentState = MODULE_STOPPED;
        }
        state = CurrentState;
        return Error();
      }

      virtual Error Reset()
      {
        Device->Reset();
        Tracker::InitState(*Data, ModState);
        PlayerState = State();
        CurrentState = MODULE_STOPPED;
        return Error();
      }

      virtual Error SetPosition(uint_t frame)
      {
        if (frame < ModState.Frame)
        {
          //reset to beginning in case of moving back
          const uint64_t keepTicks = ModState.Tick;
          Tracker::InitState(*Data, ModState);
          PlayerState = State();
          ModState.Tick = keepTicks;
        }
        //fast forward
        AYM::DataChunk chunk;
        while (ModState.Frame < frame)
        {
          //do not update tick for proper rendering
          assert(Data->Positions.size() > ModState.Track.Position);
          RenderData(chunk);
          if (!Tracker::UpdateState(*Data, ModState, Sound::LOOP_NONE))
          {
            break;
          }
        }
        return Error();
      }

    protected:
      //result processing function
      virtual void RenderData(AYM::DataChunk& chunk) = 0;
    protected:
      const typename ModuleData::ConstPtr Data;
      //typed state
      State PlayerState;
    };
  }
}

#undef FILE_TAG
#endif //__CORE_PLUGINS_PLAYERS_AYM_BASE_H_DEFINED__
