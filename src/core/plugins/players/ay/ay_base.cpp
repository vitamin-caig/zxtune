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
//common includes
#include <tools.h>
//library includes
#include <core/error_codes.h>
#include <sound/receiver.h>
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

  class AYMDataIterator : public AYM::DataIterator
  {
  public:
    AYMDataIterator(AYM::TrackParameters::Ptr trackParams, StateIterator::Ptr iterator, AYM::DataRenderer::Ptr renderer)
      : TrackParams(trackParams)
      , Iterator(iterator)
      , Render(renderer)
      , Builder(TrackParams->FreqTable())
    {
    }

    virtual void Reset()
    {
      Iterator->Reset();
      Render->Reset();
    }

    virtual bool NextFrame(bool looped)
    {
      Builder.Initialize();
      Render->SynthesizeData(*Iterator, Builder);
      return Iterator->NextFrame(looped);
    }

    virtual void GetData(Devices::AYM::DataChunk& chunk) const
    {
      Builder.GetResult(chunk);
    }
  private:
    const AYM::TrackParameters::Ptr TrackParams;
    const StateIterator::Ptr Iterator;
    const AYM::DataRenderer::Ptr Render;
    AYM::TrackBuilder Builder;
  };

  typedef boost::array<uint_t, Devices::AYM::CHANNELS> LayoutData;
  
  const LayoutData LAYOUTS[] =
  {
    { {0, 1, 2} }, //ABC
    { {0, 2, 1} }, //ACB
    { {1, 0, 2} }, //BAC
    { {2, 0, 1} }, //BCA
    { {2, 1, 0} }, //CBA
    { {1, 2, 0} }, //CAB
  };

  class AYMReceiver : public Devices::AYM::Receiver
  {
  public:
    AYMReceiver(AYM::TrackParameters::Ptr params, Sound::MultichannelReceiver::Ptr target)
    //TODO: poll layout change
      : Layout(LAYOUTS[params->Layout()])
      , Target(target)
      , Data(Devices::AYM::CHANNELS)
    {
    }

    virtual void ApplyData(const Devices::AYM::MultiSample& data)
    {
      for (uint_t idx = 0; idx < Devices::AYM::CHANNELS; ++idx)
      {
        const uint_t chipChannel = Layout[idx];
        Data[idx] = AYMSampleToSoundSample(data[chipChannel]);
      }
      Target->ApplyData(Data);
    }

    virtual void Flush()
    {
      Target->Flush();
    }
  private:
    inline static Sound::Sample AYMSampleToSoundSample(Devices::AYM::Sample in)
    {
      return in / 2;
    }
  private:
    const LayoutData& Layout;
    const Sound::MultichannelReceiver::Ptr Target;
    std::vector<Sound::Sample> Data;
  };

  class AYMAnalyzer : public Analyzer
  {
  public:
    explicit AYMAnalyzer(Devices::AYM::Chip::Ptr device)
      : Device(device)
    {
    }

    //analyzer virtuals
    virtual uint_t ActiveChannels() const
    {
      Devices::AYM::ChannelsState state;
      Device->GetState(state);
      return static_cast<uint_t>(std::count_if(state.begin(), state.end(),
        boost::mem_fn(&Devices::AYM::ChanState::Enabled)));
    }

    virtual void BandLevels(std::vector<std::pair<uint_t, uint_t> >& bandLevels) const
    {
      Devices::AYM::ChannelsState state;
      Device->GetState(state);
      bandLevels.resize(state.size());
      std::transform(state.begin(), state.end(), bandLevels.begin(),
        boost::bind(&std::make_pair<uint_t, uint_t>, boost::bind(&Devices::AYM::ChanState::Band, _1), boost::bind(&Devices::AYM::ChanState::LevelInPercents, _1)));
    }
  private:
    const Devices::AYM::Chip::Ptr Device;
  };

  class AYMRenderer : public Renderer
  {
  public:
    AYMRenderer(TrackState::Ptr state, AYM::DataIterator::Ptr iterator, Devices::AYM::Chip::Ptr device)
      : State(state)
      , Iterator(iterator)
      , Device(device)
      , LastRenderTick(0)
    {
#ifndef NDEBUG
//perform self-test
      while (Iterator->NextFrame(false)) {}
      Reset();
#endif
    }

    virtual TrackState::Ptr GetTrackState() const
    {
      return State;
    }

    virtual Analyzer::Ptr GetAnalyzer() const
    {
      return boost::make_shared<AYMAnalyzer>(Device);
    }

    virtual bool RenderFrame(const Sound::RenderParameters& params)
    {
      const bool res = Iterator->NextFrame(params.Looped());
      LastRenderTick += params.ClocksPerFrame();

      Devices::AYM::DataChunk chunk;
      Iterator->GetData(chunk);
      chunk.Tick = LastRenderTick;
      Device->RenderData(chunk);
      return res;
    }

    virtual void Reset()
    {
      Iterator->Reset();
      Device->Reset();
      LastRenderTick = 0;
    }

    virtual void SetPosition(uint_t frameNum)
    {
      if (State->Frame() > frameNum)
      {
        Iterator->Reset();
      }
      while (State->Frame() < frameNum)
      {
        if (!Iterator->NextFrame(false))
        {
          break;
        }
      }
    }
  private:
    const TrackState::Ptr State;
    const AYM::DataIterator::Ptr Iterator;
    const Devices::AYM::Chip::Ptr Device;
    uint64_t LastRenderTick;
  };
}

namespace ZXTune
{
  namespace Module
  {
    namespace AYM
    {
      void ChannelBuilder::SetTone(int_t halfTones, int_t offset)
      {
        const int_t halftone = clamp<int_t>(halfTones, 0, static_cast<int_t>(Table.size()) - 1);
        const uint_t tone = (Table[halftone] + offset) & 0xfff;

        const uint_t reg = Devices::AYM::DataChunk::REG_TONEA_L + 2 * Channel;
        Chunk.Data[reg] = static_cast<uint8_t>(tone & 0xff);
        Chunk.Data[reg + 1] = static_cast<uint8_t>(tone >> 8);
        Chunk.Mask |= (1 << reg) | (1 << (reg + 1));
      }

      void ChannelBuilder::SetLevel(int_t level)
      {
        const uint_t reg = Devices::AYM::DataChunk::REG_VOLA + Channel;
        Chunk.Data[reg] = static_cast<uint8_t>(clamp<int_t>(level, 0, 15));
        Chunk.Mask |= 1 << reg;
      }

      void ChannelBuilder::DisableTone()
      {
        Chunk.Data[Devices::AYM::DataChunk::REG_MIXER] |= (Devices::AYM::DataChunk::REG_MASK_TONEA << Channel);
        Chunk.Mask |= 1 << Devices::AYM::DataChunk::REG_MIXER;
      }

      void ChannelBuilder::EnableEnvelope()
      {
        const uint_t reg = Devices::AYM::DataChunk::REG_VOLA + Channel;
        Chunk.Data[reg] |= Devices::AYM::DataChunk::REG_MASK_ENV;
        Chunk.Mask |= 1 << reg;
      }

      void ChannelBuilder::DisableNoise()
      {
        Chunk.Data[Devices::AYM::DataChunk::REG_MIXER] |= (Devices::AYM::DataChunk::REG_MASK_NOISEA << Channel);
        Chunk.Mask |= 1 << Devices::AYM::DataChunk::REG_MIXER;
      }

      void TrackBuilder::SetNoise(uint_t level)
      {
        Chunk.Data[Devices::AYM::DataChunk::REG_TONEN] = level & 31;
        Chunk.Mask |= 1 << Devices::AYM::DataChunk::REG_TONEN;
      }

      void TrackBuilder::SetEnvelopeType(uint_t type)
      {
        Chunk.Data[Devices::AYM::DataChunk::REG_ENV] = static_cast<uint8_t>(type);
        Chunk.Mask |= 1 << Devices::AYM::DataChunk::REG_ENV;
      }

      void TrackBuilder::SetEnvelopeTone(uint_t tone)
      {
        Chunk.Data[Devices::AYM::DataChunk::REG_TONEE_L] = static_cast<uint8_t>(tone & 0xff);
        Chunk.Data[Devices::AYM::DataChunk::REG_TONEE_H] = static_cast<uint8_t>(tone >> 8);
        Chunk.Mask |= (1 << Devices::AYM::DataChunk::REG_TONEE_L) | (1 << Devices::AYM::DataChunk::REG_TONEE_H);
      }

      int_t TrackBuilder::GetSlidingDifference(int_t halfToneFrom, int_t halfToneTo) const
      {
        const int_t halfFrom = clamp<int_t>(halfToneFrom, 0, static_cast<int_t>(Table.size()) - 1);
        const int_t halfTo = clamp<int_t>(halfToneTo, 0, static_cast<int_t>(Table.size()) - 1);
        const int_t toneFrom = Table[halfFrom];
        const int_t toneTo = Table[halfTo];
        return toneTo - toneFrom;
      }

      void TrackBuilder::SetRawChunk(const Devices::AYM::DataChunk& chunk)
      {
        std::copy(chunk.Data.begin(), chunk.Data.end(), Chunk.Data.begin());
        Chunk.Mask &= ~Devices::AYM::DataChunk::MASK_ALL_REGISTERS;
        Chunk.Mask |= chunk.Mask & Devices::AYM::DataChunk::MASK_ALL_REGISTERS;
      }

      DataIterator::Ptr CreateDataIterator(AYM::TrackParameters::Ptr trackParams, StateIterator::Ptr iterator, DataRenderer::Ptr renderer)
      {
        return boost::make_shared<AYMDataIterator>(trackParams, iterator, renderer);
      }

      Renderer::Ptr CreateRenderer(TrackState::Ptr state, AYM::DataIterator::Ptr iterator, Devices::AYM::Chip::Ptr device)
      {
        return boost::make_shared<AYMRenderer>(state, iterator, device);
      }

      Devices::AYM::Receiver::Ptr CreateReceiver(TrackParameters::Ptr params, Sound::MultichannelReceiver::Ptr target)
      {
        return boost::make_shared<AYMReceiver>(params, target);
      }

      Renderer::Ptr CreateRenderer(AYM::TrackParameters::Ptr params, StateIterator::Ptr iterator, DataRenderer::Ptr renderer, Devices::AYM::Chip::Ptr device)
      {
        const DataIterator::Ptr dataIter = CreateDataIterator(params, iterator, renderer);
        return CreateRenderer(iterator, dataIter, device);
      }

      Renderer::Ptr CreateStreamRenderer(TrackParameters::Ptr params, Information::Ptr info, DataRenderer::Ptr renderer, Devices::AYM::Chip::Ptr device)
      {
        const StateIterator::Ptr iterator = CreateStreamStateIterator(info);
        return CreateRenderer(params, iterator, renderer, device);
      }

      Renderer::Ptr CreateTrackRenderer(TrackParameters::Ptr params, Information::Ptr info, TrackModuleData::Ptr data, 
        DataRenderer::Ptr renderer, Devices::AYM::Chip::Ptr device)
      {
        const StateIterator::Ptr iterator = CreateTrackStateIterator(info, data);
        return CreateRenderer(params, iterator, renderer, device);
      }
    }
  }
}
