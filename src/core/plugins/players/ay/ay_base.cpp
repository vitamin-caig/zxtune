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

  class AYMPlayer : public Player
  {
  public:
    AYMPlayer(AYM::ParametersHelper::Ptr params, StateIterator::Ptr iterator, AYMDataRenderer::Ptr renderer, AYM::Chip::Ptr device)
      : Renderer(renderer)
      , Device(device)
      , AYMHelper(params)
      , Iterator(iterator)
      , CurrentState(MODULE_STOPPED)
    {
#ifndef NDEBUG
//perform self-test
      Reset();
      AYMTrackSynthesizer synthesizer(*AYMHelper);
      do
      {
        Renderer->SynthesizeData(*Iterator, synthesizer);
      }
      while (Iterator->NextFrame(0, Sound::LOOP_NONE));
#endif
      Reset();
    }

    virtual TrackState::Ptr GetTrackState() const
    {
      return Iterator;
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

      synthesizer.InitData(Iterator->AbsoluteTick() + ticksDelta);
      Renderer->SynthesizeData(*Iterator, synthesizer);

      CurrentState = Iterator->NextFrame(ticksDelta, params.Looping())
        ? MODULE_PLAYING : MODULE_STOPPED;

      const AYM::DataChunk& chunk = synthesizer.GetData();
      Device->RenderData(params, chunk);
      state = CurrentState;
      return Error();
    }

    virtual Error Reset()
    {
      Device->Reset();
      Iterator->Reset();
      Renderer->Reset();
      CurrentState = MODULE_STOPPED;
      return Error();
    }

    virtual Error SetPosition(uint_t frame)
    {
      if (frame < Iterator->Frame())
      {
        //reset to beginning in case of moving back
        Iterator->ResetPosition();
        Renderer->Reset();
      }
      //fast forward
      AYMTrackSynthesizer synthesizer(*AYMHelper);
      while (Iterator->Frame() < frame)
      {
        //do not update tick for proper rendering
        Renderer->SynthesizeData(*Iterator, synthesizer);
        if (!Iterator->NextFrame(0, Sound::LOOP_NONE))
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
    const StateIterator::Ptr Iterator;
    PlaybackState CurrentState;
  };
}

namespace ZXTune
{
  namespace Module
  {
    Player::Ptr CreateAYMPlayer(AYM::ParametersHelper::Ptr params, StateIterator::Ptr iterator, AYMDataRenderer::Ptr renderer, AYM::Chip::Ptr device)
    {
      return boost::make_shared<AYMPlayer>(params, iterator, renderer, device);
    }

    Player::Ptr CreateAYMStreamPlayer(Information::Ptr info, AYMDataRenderer::Ptr renderer, AYM::Chip::Ptr device)
    {
      const AYM::ParametersHelper::Ptr params = AYM::ParametersHelper::Create(TABLE_SOUNDTRACKER);
      params->SetParameters(*info->Properties());
      const StateIterator::Ptr iterator = CreateStreamStateIterator(info);
      return CreateAYMPlayer(params, iterator, renderer, device);
    }

    Player::Ptr CreateAYMTrackPlayer(Information::Ptr info, TrackModuleData::Ptr data, 
      AYMDataRenderer::Ptr renderer, AYM::Chip::Ptr device, const String& defaultTable)
    {
      const AYM::ParametersHelper::Ptr params = AYM::ParametersHelper::Create(defaultTable);
      params->SetParameters(*info->Properties());
      const StateIterator::Ptr iterator = CreateTrackStateIterator(info, data);
      return CreateAYMPlayer(params, iterator, renderer, device);
    }
  }
}
