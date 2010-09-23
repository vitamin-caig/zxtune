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
#include "../tracking.h"
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
      inline static Analyze::Channel AnalyzeAYState(const AYM::ChanState& ayState)
      {
        Analyze::Channel res;
        res.Enabled = ayState.Enabled;
        res.Band = ayState.Band;
        res.Level = ayState.LevelInPercents * std::numeric_limits<Analyze::LevelType>::max() / 100;
        return res;
      }
    public:
      AYMPlayer(Information::Ptr info, typename ModuleData::Ptr data,
        AYM::Chip::Ptr device, const String& defTable)
        : Info(info)
        , Data(data)
        , AYMHelper(AYM::ParametersHelper::Create(defTable))
        , Device(device)
        , CurrentState(MODULE_STOPPED)
      {
        //WARNING: not a virtual call
        Reset();
      }

      virtual Information::Ptr GetInformation() const
      {
        return Info;
      }

      virtual TrackState::Ptr GetTrackState() const
      {
        return boost::make_shared<StubTrackState>(ModState);
      }

      virtual void GetAnalyzer(Analyze::ChannelsState& analyzeState) const
      {
        AYM::ChannelsState devState;
        Device->GetState(devState);
        analyzeState.resize(devState.size());
        std::transform(devState.begin(), devState.end(), analyzeState.begin(), &AnalyzeAYState);
      }

      virtual Error SetParameters(const Parameters::Accessor& params)
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
        AYMTrackSynthesizer synthesizer(*AYMHelper);
        ModState.Tick += params.ClocksPerFrame();

        synthesizer.InitData(ModState.Tick);
        SynthesizeData(synthesizer);

        const AYM::DataChunk& chunk = synthesizer.GetData();

        Device->RenderData(params, chunk, receiver);
        if (Data->UpdateState(*Info, params.Looping, ModState))
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
        Data->InitState(Info->Tempo(), Info->FramesCount(), ModState);
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
          Data->InitState(Info->Tempo(), Info->FramesCount(), ModState);
          PlayerState = InternalState();
          ModState.Tick = keepTicks;
        }
        //fast forward
        AYMTrackSynthesizer synthesizer(*AYMHelper);
        while (ModState.Track.Frame < frame)
        {
          //do not update tick for proper rendering
          SynthesizeData(synthesizer);
          if (!Data->UpdateState(*Info, Sound::LOOP_NONE, ModState))
          {
            break;
          }
        }
        ModState.Frame = frame;
        return Error();
      }

    protected:
      bool IsNewLine() const
      {
        return 0 == ModState.Track.Quirk;
      }

      bool IsNewPattern() const
      {
        return 0 == ModState.Track.Line && IsNewLine();
      }
      //result processing function
      virtual void SynthesizeData(AYMTrackSynthesizer& synthesizer) = 0;
    protected:
      const Information::Ptr Info;
      const typename ModuleData::Ptr Data;
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
#endif //__CORE_PLUGINS_PLAYERS_AY_BASE_H_DEFINED__
