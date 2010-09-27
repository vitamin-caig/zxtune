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

    //Common base for all aym-based players (interpret as a tracked if required)
    //ModuleData - source data type
    //InternalState - internal state type
    template<class InternalState>
    class AYMPlayer : public Player
    {
    public:
      AYMPlayer(Information::Ptr info, TrackModuleData::Ptr data,
        AYM::Chip::Ptr device, const String& defTable)
        : Info(info)
        , Device(AYMDevice::Create(device))
        , AYMHelper(AYM::ParametersHelper::Create(defTable))
        , StateIterator(TrackStateIterator::Create(Info, data, Device))
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
        return StateIterator;
      }

      virtual Analyzer::Ptr GetAnalyzer() const
      {
        return Device;
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
        const uint64_t ticksDelta = params.ClocksPerFrame();

        AYMTrackSynthesizer synthesizer(*AYMHelper);

        synthesizer.InitData(StateIterator->AbsoluteTick() + ticksDelta);
        SynthesizeData(synthesizer);

        CurrentState = StateIterator->NextFrame(ticksDelta, params.Looping)
          ? MODULE_PLAYING : MODULE_STOPPED;

        const AYM::DataChunk& chunk = synthesizer.GetData();
        Device->RenderData(params, chunk, receiver);

        if (MODULE_STOPPED == CurrentState)
        {
          receiver.Flush();
        }
        state = CurrentState;
        return Error();
      }

      virtual Error Reset()
      {
        Device->Reset();
        StateIterator->Reset();
        PlayerState = InternalState();
        CurrentState = MODULE_STOPPED;
        return Error();
      }

      virtual Error SetPosition(uint_t frame)
      {
        frame = std::min(frame, Info->FramesCount() - 1);
        if (frame < StateIterator->Frame())
        {
          //reset to beginning in case of moving back
          StateIterator->ResetPosition();
          PlayerState = InternalState();
        }
        //fast forward
        AYMTrackSynthesizer synthesizer(*AYMHelper);
        while (StateIterator->Frame() < frame)
        {
          //do not update tick for proper rendering
          SynthesizeData(synthesizer);
          if (!StateIterator->NextFrame(0, Sound::LOOP_NONE))
          {
            break;
          }
        }
        return Error();
      }

    protected:
      bool IsNewLine() const
      {
        return 0 == StateIterator->Quirk();
      }

      bool IsNewPattern() const
      {
        return 0 == StateIterator->Line() && IsNewLine();
      }
      //result processing function
      virtual void SynthesizeData(AYMTrackSynthesizer& synthesizer) = 0;
    private:
      const Information::Ptr Info;
      const AYMDevice::Ptr Device;
    protected:
      //aym-related
      AYM::ParametersHelper::Ptr AYMHelper;
      //tracking-related
      const TrackStateIterator::Ptr StateIterator;
      //typed state
      InternalState PlayerState;
    private:
      PlaybackState CurrentState;
    };
  }
}

#undef FILE_TAG
#endif //__CORE_PLUGINS_PLAYERS_AY_BASE_H_DEFINED__
