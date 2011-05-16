/*
Abstract:
  AYM-based players common functionality implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "ay_base.h"
//library includes
#include <core/error_codes.h>
//std includes
#include <limits>
//boost includes
#include <boost/bind.hpp>
#include <boost/mem_fn.hpp>
//text includes
#include <core/text/core.h>

#define FILE_TAG 7D6A549C

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  class AYMAnalyzer : public Analyzer
  {
  public:
    explicit AYMAnalyzer(AYM::Chip::Ptr device)
      : Device(device)
    {
    }

    //analyzer virtuals
    virtual uint_t ActiveChannels() const
    {
      AYM::ChannelsState state;
      Device->GetState(state);
      return static_cast<uint_t>(std::count_if(state.begin(), state.end(),
        boost::mem_fn(&AYM::ChanState::Enabled)));
    }

    virtual void BandLevels(std::vector<std::pair<uint_t, uint_t> >& bandLevels) const
    {
      AYM::ChannelsState state;
      Device->GetState(state);
      bandLevels.resize(state.size());
      std::transform(state.begin(), state.end(), bandLevels.begin(),
        boost::bind(&std::make_pair<uint_t, uint_t>, boost::bind(&AYM::ChanState::Band, _1), boost::bind(&AYM::ChanState::LevelInPercents, _1)));
    }
  private:
    const AYM::Chip::Ptr Device;
  };

  class AYMStreamPlayer : public Player
  {
  public:
    AYMStreamPlayer(Information::Ptr info, AYMDataRenderer::Ptr renderer, AYM::Chip::Ptr device)
      : Renderer(renderer)
      , Device(device)
      , AYMHelper(AYM::ParametersHelper::Create(TABLE_SOUNDTRACKER))
      , StateIterator(StreamStateIterator::Create(info))
      , CurrentState(MODULE_STOPPED)
    {
      AYMHelper->SetParameters(*info->Properties());
      Reset();
    }

    virtual TrackState::Ptr GetTrackState() const
    {
      return StateIterator;
    }

    virtual Analyzer::Ptr GetAnalyzer() const
    {
      return boost::make_shared<AYMAnalyzer>(Device);
    }

    virtual Error RenderFrame(const Sound::RenderParameters& params,
                              PlaybackState& state)
    {
      const uint64_t ticksDelta = params.ClocksPerFrame();

      AYMTrackSynthesizer synthesizer(*AYMHelper);

      synthesizer.InitData(StateIterator->AbsoluteTick() + ticksDelta);
      Renderer->SynthesizeData(*StateIterator, synthesizer);

      CurrentState = StateIterator->NextFrame(ticksDelta, params.Looping())
        ? MODULE_PLAYING : MODULE_STOPPED;

      const AYM::DataChunk& chunk = synthesizer.GetData();
      Device->RenderData(params, chunk);
      state = CurrentState;
      return Error();
    }

    virtual Error Reset()
    {
      Device->Reset();
      StateIterator->Reset();
      Renderer->Reset();
      CurrentState = MODULE_STOPPED;
      return Error();
    }

    virtual Error SetPosition(uint_t frame)
    {
      if (frame < StateIterator->Frame())
      {
        //reset to beginning in case of moving back
        StateIterator->ResetPosition();
        Renderer->Reset();
      }
      //fast forward
      AYMTrackSynthesizer synthesizer(*AYMHelper);
      while (StateIterator->Frame() < frame)
      {
        //do not update tick for proper rendering
        Renderer->SynthesizeData(*StateIterator, synthesizer);
        if (!StateIterator->NextFrame(0, Sound::LOOP_NONE))
        {
          break;
        }
      }
      return Error();
    }
  private:
    const AYMDataRenderer::Ptr Renderer;
    const AYM::Chip::Ptr Device;
    const AYM::ParametersHelper::Ptr AYMHelper;
    const StreamStateIterator::Ptr StateIterator;
    PlaybackState CurrentState;
  };

  class AYMTrackPlayer : public Player
  {
  public:
    AYMTrackPlayer(Information::Ptr info, TrackModuleData::Ptr data, 
      AYMDataRenderer::Ptr renderer, AYM::Chip::Ptr device, const String& defaultTable)
      : Renderer(renderer)
      , Device(device)
      , AYMHelper(AYM::ParametersHelper::Create(defaultTable))
      , StateIterator(TrackStateIterator::Create(info, data))
      , CurrentState(MODULE_STOPPED)
    {
      AYMHelper->SetParameters(*info->Properties());
#ifndef NDEBUG
//perform self-test
      AYMTrackSynthesizer synthesizer(*AYMHelper);
      do
      {
        Renderer->SynthesizeData(*StateIterator, synthesizer);
      }
      while (StateIterator->NextFrame(0, Sound::LOOP_NONE));
      Reset();
#endif
    }

    virtual TrackState::Ptr GetTrackState() const
    {
      return StateIterator;
    }

    virtual Analyzer::Ptr GetAnalyzer() const
    {
      return boost::make_shared<AYMAnalyzer>(Device);
    }

    virtual Error RenderFrame(const Sound::RenderParameters& params,
                              PlaybackState& state)
    {
      const uint64_t ticksDelta = params.ClocksPerFrame();

      AYMTrackSynthesizer synthesizer(*AYMHelper);

      synthesizer.InitData(StateIterator->AbsoluteTick() + ticksDelta);
      Renderer->SynthesizeData(*StateIterator, synthesizer);

      CurrentState = StateIterator->NextFrame(ticksDelta, params.Looping())
        ? MODULE_PLAYING : MODULE_STOPPED;

      const AYM::DataChunk& chunk = synthesizer.GetData();
      Device->RenderData(params, chunk);
      state = CurrentState;
      return Error();
    }

    virtual Error Reset()
    {
      Device->Reset();
      StateIterator->Reset();
      Renderer->Reset();
      CurrentState = MODULE_STOPPED;
      return Error();
    }

    virtual Error SetPosition(uint_t frame)
    {
      if (frame < StateIterator->Frame())
      {
        //reset to beginning in case of moving back
        StateIterator->ResetPosition();
        Renderer->Reset();
      }
      //fast forward
      AYMTrackSynthesizer synthesizer(*AYMHelper);
      while (StateIterator->Frame() < frame)
      {
        //do not update tick for proper rendering
        Renderer->SynthesizeData(*StateIterator, synthesizer);
        if (!StateIterator->NextFrame(0, Sound::LOOP_NONE))
        {
          break;
        }
      }
      return Error();
    }

  private:
    const AYMDataRenderer::Ptr Renderer;
    const AYM::Chip::Ptr Device;
    const AYM::ParametersHelper::Ptr AYMHelper;
    const TrackStateIterator::Ptr StateIterator;
    PlaybackState CurrentState;
  };
}

namespace ZXTune
{
  namespace Module
  {
    Player::Ptr CreateAYMStreamPlayer(Information::Ptr info, AYMDataRenderer::Ptr renderer, AYM::Chip::Ptr device)
    {
      return boost::make_shared<AYMStreamPlayer>(info, renderer, device);
    }

    Player::Ptr CreateAYMTrackPlayer(Information::Ptr info, TrackModuleData::Ptr data, 
      AYMDataRenderer::Ptr renderer, AYM::Chip::Ptr device, const String& defaultTable)
    {
      return boost::make_shared<AYMTrackPlayer>(info, data, renderer, device, defaultTable);
    }
  }
}
