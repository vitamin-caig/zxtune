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
#include "aym_parameters_helper.h"
//common includes
#include <error.h>
//library includes
#include <core/error_codes.h>
#include <core/module_holder.h>
#include <devices/aym.h>
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
    //Common base for all aym-based players (interpret as a tracked if required)
    //ModuleData - source data type
    //InternalState - internal state type
    template<class ModuleData, class InternalState>
    class AYMPlayer : public Player
    {
    public:
      AYMPlayer(typename ModuleData::ConstPtr data,
        AYM::Chip::Ptr device, const String& defTable)
        : Data(data)
        , AYMHelper(AYM::ParametersHelper::Create(defTable))
        , Device(device)
        , CurrentState(MODULE_STOPPED)
      {
        //WARNING: not a virtual call
        Reset();
      }

      virtual Error GetPlaybackState(Module::State& state,
                                     Analyze::ChannelsState& analyzeState) const
      {
        state = ModState;
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

      virtual Error RenderFrame(const Sound::RenderParameters& params,
                                PlaybackState& state,
                                Sound::MultichannelReceiver& receiver)
      {
        AYM::DataChunk chunk;
        AYMHelper->GetDataChunk(chunk);
        ModState.Tick += params.ClocksPerFrame();
        chunk.Tick = ModState.Tick;
        RenderData(chunk);

        Device->RenderData(params, chunk, receiver);
        if (Data->UpdateState(ModState, params.Looping))
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
        Data->InitState(ModState);
        PlayerState = InternalState();
        CurrentState = MODULE_STOPPED;
        return Error();
      }

      virtual Error SetPosition(uint_t frame)
      {
        frame = std::min(frame, ModState.Reference.Frame - 1);
        if (frame < ModState.Track.Frame)
        {
          //reset to beginning in case of moving back
          const uint64_t keepTicks = ModState.Tick;
          Data->InitState(ModState);
          PlayerState = InternalState();
          ModState.Tick = keepTicks;
        }
        //fast forward
        AYM::DataChunk chunk;
        while (ModState.Track.Frame < frame)
        {
          //do not update tick for proper rendering
          RenderData(chunk);
          if (!Data->UpdateState(ModState, Sound::LOOP_NONE))
          {
            break;
          }
        }
        ModState.Frame = frame;
        return Error();
      }

    protected:
      //result processing function
      virtual void RenderData(AYM::DataChunk& chunk) = 0;
    protected:
      const typename ModuleData::ConstPtr Data;
      //aym-related
      AYM::ParametersHelper::Ptr AYMHelper;
      //tracking-related
      Module::State ModState;
      //typed state
      InternalState PlayerState;
    private:
      AYM::Chip::Ptr Device;
      PlaybackState CurrentState;
    };
  }
}

#undef FILE_TAG
#endif //__CORE_PLUGINS_PLAYERS_AYM_BASE_H_DEFINED__
