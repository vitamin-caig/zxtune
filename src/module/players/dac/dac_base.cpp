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

#include "sound/mixer_factory.h"
#include "sound/multichannel_sample.h"

#include "make_ptr.h"

namespace Module
{
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
      return ToState(Iterator->GetState(), FrameDuration.CastTo<Time::Microsecond>());
    }

    Sound::Chunk Render() override
    {
      TransferChunk();
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
        LastChunk.TimeStamp += FrameDuration;
      }
    }

  private:
    void TransferChunk()
    {
      LastChunk.Data = Iterator->Advance();
      Device->RenderData(LastChunk);
    }

  private:
    const DAC::DataIterator::Ptr Iterator;
    const Devices::DAC::Chip::Ptr Device;
    const Time::Duration<Devices::DAC::TimeUnit> FrameDuration;
    Devices::DAC::DataChunk LastChunk;
  };

  class DACHolder : public Holder
  {
  public:
    DACHolder(DAC::Chiptune::Ptr chiptune, Parameters::Accessor::Ptr properties)
      : Tune(std::move(chiptune))
      , Properties(std::move(properties))
    {}

    Information GetModuleInformation() const override
    {
      return ToInformation(CreateTrackInfo(*Tune->GetTrackModel()), Tune->GetFrameDuration());
    }

    Parameters::Accessor::Ptr GetModuleProperties() const override
    {
      return Properties;
    }

    Renderer::Ptr CreateRenderer(uint_t samplerate, Parameters::Accessor::Ptr params) const override
    {
      auto iterator = Tune->CreateDataIterator();
      auto chip = DAC::CreateChip(Tune->GetTrackModel()->GetChannelsCount(), samplerate, std::move(params));
      Tune->GetSamples(*chip);
      return DAC::CreateRenderer(Tune->GetFrameDuration() /*TODO: speed variation*/, std::move(iterator),
                                 std::move(chip));
    }

  private:
    const DAC::Chiptune::Ptr Tune;
    const Parameters::Accessor::Ptr Properties;
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

  Renderer::Ptr CreateRenderer(Time::Microseconds frameDuration, DAC::DataIterator::Ptr iterator,
                               Devices::DAC::Chip::Ptr device)
  {
    return MakePtr<DACRenderer>(frameDuration, std::move(iterator), std::move(device));
  }

  template<unsigned Channels>
  Devices::DAC::Chip::Ptr CreateChip(uint_t samplerate, Parameters::Accessor::Ptr params)
  {
    using MixerType = Sound::FixedChannelsMatrixMixer<Channels>;
    auto mixer = MixerType::Create();
    auto pollParams = Sound::CreateMixerNotificationParameters(std::move(params), mixer);
    auto chipParams = ChipParameters::Create(samplerate, std::move(pollParams));
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
    return MakePtr<DACHolder>(std::move(chiptune), std::move(properties));
  }
}  // namespace Module::DAC
