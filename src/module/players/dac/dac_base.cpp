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

#include "module/players/dac/dac_parameters.h"
#include "module/players/parameters.h"

#include "sound/mixer_factory.h"
#include "sound/multichannel_sample.h"

#include "make_ptr.h"

namespace Module
{
  class DACRenderer : public Renderer
  {
  public:
    DACRenderer(FrameDurationParameter frameDuration, DAC::DataIterator::Ptr iterator, Devices::DAC::Chip::Ptr device)
      : FrameDuration(std::move(frameDuration))
      , Iterator(std::move(iterator))
      , Device(std::move(device))
    {}

    State GetState() const override
    {
      return ToState(Iterator->GetState(), FrameDuration.Get());
    }

    Sound::Chunk Render() override
    {
      TransferChunk();
      FrameDuration.Synchronize();
      LastChunk.TimeStamp += FrameDuration.Get<Devices::DAC::TimeUnit>();
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
      if (request < GetState().At)
      {
        Iterator->Reset();
        Device->Reset();
        LastChunk.TimeStamp = {};
      }
      while (GetState().At < request)
      {
        LastChunk.Data = Iterator->Advance();
        Device->UpdateState(LastChunk);
        LastChunk.TimeStamp += FrameDuration.Get<Devices::DAC::TimeUnit>();
      }
    }

  private:
    void TransferChunk()
    {
      LastChunk.Data = Iterator->Advance();
      Device->RenderData(LastChunk);
    }

  private:
    FrameDurationParameter FrameDuration;
    const DAC::DataIterator::Ptr Iterator;
    const Devices::DAC::Chip::Ptr Device;
    Devices::DAC::DataChunk LastChunk;
  };

  class DACHolder : public Holder
  {
  public:
    DACHolder(DAC::Chiptune::Ptr chiptune, Parameters::Accessor::Ptr properties, Time::Microseconds frameDuration)
      : Tune(std::move(chiptune))
      , Properties(std::move(properties))
      , FrameDuration(frameDuration)
    {}

    Information GetModuleInformation() const override
    {
      return ToInformation(CreateTrackInfo(*Tune->GetTrackModel()), FrameDuration);
    }

    Parameters::Accessor::Ptr GetModuleProperties() const override
    {
      return Properties;
    }

    Renderer::Ptr CreateRenderer(uint_t samplerate, Parameters::Accessor::Ptr params) const override
    {
      auto rendererParams = params;
      auto iterator = Tune->CreateDataIterator();
      auto chip = DAC::CreateChip(Tune->GetTrackModel()->GetChannelsCount(), samplerate, std::move(params));
      Tune->GetSamples(*chip);
      return DAC::CreateRenderer(FrameDurationParameter(std::move(rendererParams), DAC::BASE_FRAME_DURATION),
                                 std::move(iterator), std::move(chip));
    }

  private:
    const DAC::Chiptune::Ptr Tune;
    const Parameters::Accessor::Ptr Properties;
    const Time::Microseconds FrameDuration;
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

  Renderer::Ptr CreateRenderer(FrameDurationParameter frameDuration, DAC::DataIterator::Ptr iterator,
                               Devices::DAC::Chip::Ptr device)
  {
    return MakePtr<DACRenderer>(std::move(frameDuration), std::move(iterator), std::move(device));
  }

  template<unsigned Channels>
  Devices::DAC::Chip::Ptr CreateChip(uint_t samplerate, Parameters::Accessor::Ptr params)
  {
    using MixerType = Sound::FixedChannelsMatrixMixer<Channels>;
    auto mixer = MixerType::Create();
    auto pollParams = Sound::CreateMixerNotificationParameters(std::move(params), mixer);
    auto chipParams = CreateChipParameters(samplerate, std::move(pollParams));
    return Devices::DAC::CreateChip(std::move(chipParams), std::move(mixer));
  }

  Devices::DAC::Chip::Ptr CreateChip(uint_t channels, uint_t samplerate, Parameters::Accessor::Ptr params)
  {
    switch (channels)
    {
    case 3:
      return CreateChip<3>(samplerate, std::move(params));
    case 4:
      return CreateChip<4>(samplerate, std::move(params));
    default:
      return {};
    };
  }

  Holder::Ptr CreateHolder(Chiptune::Ptr chiptune, Parameters::Accessor::Ptr properties)
  {
    const FrameDurationParameter frameDuration(properties, DAC::BASE_FRAME_DURATION);
    return MakePtr<DACHolder>(std::move(chiptune), std::move(properties), frameDuration.Get());
  }
}  // namespace Module::DAC
