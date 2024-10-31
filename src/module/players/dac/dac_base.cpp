/**
 *
 * @file
 *
 * @brief  DAC-based modules support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/players/dac/dac_base.h"

#include "sound/chunk.h"
#include "time/instant.h"

#include "make_ptr.h"

#include <algorithm>
#include <utility>
#include <vector>

namespace Module
{
  class DACDataIterator : public DAC::DataIterator
  {
  public:
    DACDataIterator(TrackStateIterator::Ptr delegate, DAC::DataRenderer::Ptr renderer)
      : Delegate(std::move(delegate))
      , State(Delegate->GetStateObserver())
      , Render(std::move(renderer))
    {
      FillCurrentData();
    }

    void Reset() override
    {
      Delegate->Reset();
      Render->Reset();
      FillCurrentData();
    }

    void NextFrame() override
    {
      Delegate->NextFrame();
      FillCurrentData();
    }

    Module::State::Ptr GetStateObserver() const override
    {
      return State;
    }

    void GetData(Devices::DAC::Channels& res) const override
    {
      res.assign(CurrentData.begin(), CurrentData.end());
    }

  private:
    void FillCurrentData()
    {
      DAC::TrackBuilder builder;
      Render->SynthesizeData(*State, builder);
      builder.GetResult(CurrentData);
    }

  private:
    const TrackStateIterator::Ptr Delegate;
    const TrackModelState::Ptr State;
    const DAC::DataRenderer::Ptr Render;
    Devices::DAC::Channels CurrentData;
  };

  class DACRenderer : public Renderer
  {
  public:
    DACRenderer(Time::Microseconds frameDuration, DAC::DataIterator::Ptr iterator, Devices::DAC::Chip::Ptr device)
      : Iterator(std::move(iterator))
      , Device(std::move(device))
      , FrameDuration(frameDuration)
    {}

    State::Ptr GetState() const override
    {
      return Iterator->GetStateObserver();
    }

    Sound::Chunk Render() override
    {
      TransferChunk();
      Iterator->NextFrame();
      LastChunk.TimeStamp += FrameDuration;
      return Device->RenderTill(LastChunk.TimeStamp);
    }

    void Reset() override
    {
      Iterator->Reset();
      Device->Reset();
      LastChunk.TimeStamp = {};
    }

    void SetPosition(Time::AtMillisecond request) override
    {
      const auto state = GetState();
      if (request < state->At())
      {
        Iterator->Reset();
        Device->Reset();
        LastChunk.TimeStamp = {};
      }
      if (LastChunk.TimeStamp == Devices::DAC::Stamp())
      {
        Iterator->GetData(LastChunk.Data);
        Device->UpdateState(LastChunk);
      }
      while (state->At() < request)
      {
        Iterator->NextFrame();
        LastChunk.TimeStamp += FrameDuration;
        Iterator->GetData(LastChunk.Data);
        Device->UpdateState(LastChunk);
      }
    }

  private:
    void TransferChunk()
    {
      Iterator->GetData(LastChunk.Data);
      Device->RenderData(LastChunk);
    }

  private:
    const DAC::DataIterator::Ptr Iterator;
    const Devices::DAC::Chip::Ptr Device;
    const Time::Duration<Devices::DAC::TimeUnit> FrameDuration;
    Devices::DAC::DataChunk LastChunk;
  };
}  // namespace Module

namespace Module::DAC
{
  ChannelDataBuilder TrackBuilder::GetChannel(uint_t chan)
  {
    using namespace Devices::DAC;
    const auto existing =
        std::find_if(Data.begin(), Data.end(), [chan](const ChannelData& data) { return data.Channel == chan; });
    if (existing != Data.end())
    {
      return ChannelDataBuilder(*existing);
    }
    Data.emplace_back();
    ChannelData& newOne = Data.back();
    newOne.Channel = chan;
    return ChannelDataBuilder(newOne);
  }

  void TrackBuilder::GetResult(Devices::DAC::Channels& result)
  {
    using namespace Devices::DAC;
    const auto last = std::remove_if(Data.begin(), Data.end(), [](const ChannelData& data) { return data.Mask == 0; });
    result.assign(Data.begin(), last);
  }

  DataIterator::Ptr CreateDataIterator(TrackStateIterator::Ptr iterator, DataRenderer::Ptr renderer)
  {
    return MakePtr<DACDataIterator>(std::move(iterator), std::move(renderer));
  }

  Renderer::Ptr CreateRenderer(Time::Microseconds frameDuration, DAC::DataIterator::Ptr iterator,
                               Devices::DAC::Chip::Ptr device)
  {
    return MakePtr<DACRenderer>(frameDuration, std::move(iterator), std::move(device));
  }
}  // namespace Module::DAC
