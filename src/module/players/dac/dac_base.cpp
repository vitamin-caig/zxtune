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

#include "sound/multichannel_sample.h"

#include "make_ptr.h"

namespace Module
{
  class DACDataIterator : public DAC::DataIterator
  {
  public:
    DACDataIterator(Iterator::Ptr delegate, DAC::DataRenderer::Ptr renderer)
      : Delegate(std::move(delegate))
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

    Module::State GetState() const override
    {
      return Delegate->GetState();
    }

    void GetData(Devices::DAC::Channels& res) const override
    {
      res.assign(CurrentData.begin(), CurrentData.end());
    }

  private:
    void FillCurrentData()
    {
      DAC::TrackBuilder builder;
      Render->SynthesizeData(*Delegate->GetState().Track, builder);
      builder.GetResult(CurrentData);
    }

  private:
    const Iterator::Ptr Delegate;
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

    State GetState() const override
    {
      return Iterator->GetState();
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
      if (request < Iterator->GetState().At)
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
      while (Iterator->GetState().At < request)
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

  DataIterator::Ptr CreateDataIterator(Iterator::Ptr iterator, DataRenderer::Ptr renderer)
  {
    return MakePtr<DACDataIterator>(std::move(iterator), std::move(renderer));
  }

  Renderer::Ptr CreateRenderer(Time::Microseconds frameDuration, DAC::DataIterator::Ptr iterator,
                               Devices::DAC::Chip::Ptr device)
  {
    return MakePtr<DACRenderer>(frameDuration, std::move(iterator), std::move(device));
  }
}  // namespace Module::DAC
