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

  class AYMDeviceImpl : public AYMDevice
  {
  public:
    explicit AYMDeviceImpl(AYM::Chip::Ptr device)
      : Device(device)
      , CurState(0)
    {
    }

    //analyzer virtuals
    virtual uint_t ActiveChannels() const
    {
      FillState();
      return std::count_if(StateCache.begin(), StateCache.end(),
        boost::mem_fn(&AYM::ChanState::Enabled));
    }

    virtual void BandLevels(std::vector<std::pair<uint_t, uint_t> >& bandLevels) const
    {
      FillState();
      bandLevels.resize(StateCache.size());
      std::transform(StateCache.begin(), StateCache.end(), bandLevels.begin(),
        boost::bind(&std::make_pair<uint_t, uint_t>, boost::bind(&AYM::ChanState::Band, _1), boost::bind(&AYM::ChanState::LevelInPercents, _1)));
    }

    virtual void RenderData(const Sound::RenderParameters& params,
                            const AYM::DataChunk& src,
                            Sound::MultichannelReceiver& dst)
    {
      Device->RenderData(params, src, dst);
      CurState = 0;
    }

    virtual void Reset()
    {
      Device->Reset();
      CurState = 0;
    }
  private:
    void FillState() const
    {
      if (!CurState)
      {
        Device->GetState(StateCache);
        CurState = &StateCache;
      }
    }
  private:
    const AYM::Chip::Ptr Device;
    mutable AYM::ChannelsState* CurState;
    mutable AYM::ChannelsState StateCache;
  };

  class AYMStreamPlayer : public Player
  {
  public:
    AYMStreamPlayer(Information::Ptr info, AYMDataRenderer::Ptr renderer, AYMDevice::Ptr device)
      : Info(info)
      , Renderer(renderer)
      , Device(device)
      , AYMHelper(AYM::ParametersHelper::Create(TABLE_SOUNDTRACKER))
      , StateIterator(StreamStateIterator::Create(Info, Device))
      , CurrentState(MODULE_STOPPED)
    {
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
      Renderer->SynthesizeData(*StateIterator, synthesizer);

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
      Renderer->Reset();
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
    const Information::Ptr Info;
    const AYMDataRenderer::Ptr Renderer;
    const AYMDevice::Ptr Device;
    const AYM::ParametersHelper::Ptr AYMHelper;
    const StreamStateIterator::Ptr StateIterator;
    PlaybackState CurrentState;
  };

  class AYMTrackPlayer : public Player
  {
  public:
    AYMTrackPlayer(Information::Ptr info, TrackModuleData::Ptr data, 
      AYMDataRenderer::Ptr renderer, AYMDevice::Ptr device, const String& defaultTable)
      : Info(info)
      , Renderer(renderer)
      , Device(device)
      , AYMHelper(AYM::ParametersHelper::Create(defaultTable))
      , StateIterator(TrackStateIterator::Create(Info, data, Device))
      , CurrentState(MODULE_STOPPED)
    {
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
      Renderer->SynthesizeData(*StateIterator, synthesizer);

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
      Renderer->Reset();
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
    const Information::Ptr Info;
    const AYMDataRenderer::Ptr Renderer;
    const AYMDevice::Ptr Device;
    const AYM::ParametersHelper::Ptr AYMHelper;
    const TrackStateIterator::Ptr StateIterator;
    PlaybackState CurrentState;
  };
}

namespace ZXTune
{
  namespace Module
  {
    AYMDevice::Ptr AYMDevice::Create(AYM::Chip::Ptr device)
    {
      return AYMDevice::Ptr(new AYMDeviceImpl(device));
    }

    Player::Ptr CreateAYMStreamPlayer(Information::Ptr info, AYMDataRenderer::Ptr renderer, AYM::Chip::Ptr device)
    {
      const AYMDevice::Ptr wrappedDevice = AYMDevice::Create(device);
      return boost::make_shared<AYMStreamPlayer>(info, renderer, wrappedDevice);
    }

    Player::Ptr CreateAYMTrackPlayer(Information::Ptr info, TrackModuleData::Ptr data, 
      AYMDataRenderer::Ptr renderer, AYM::Chip::Ptr device, const String& defaultTable)
    {
      const AYMDevice::Ptr wrappedDevice = AYMDevice::Create(device);
      return boost::make_shared<AYMTrackPlayer>(info, data, renderer, wrappedDevice, defaultTable);
    }
  }
}
