/**
 *
 * @file
 *
 * @brief  AYM-based chiptunes common functionality implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/players/aym/aym_base.h"

#include "module/players/streaming.h"
#include "module/players/tracking.h"

#include "debug/log.h"
#include "math/numeric.h"
#include "sound/mixer_factory.h"

#include "make_ptr.h"

namespace Module
{
  const Debug::Stream Dbg("Core::AYBase");

  class AYMRenderer : public Renderer
  {
  public:
    AYMRenderer(Time::Microseconds frameDuration, AYM::DataIterator::Ptr iterator, Devices::AYM::Chip::Ptr device)
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
        TransferChunk();
      }
    }

  private:
    void TransferChunk()
    {
      LastChunk.Data = Iterator->Advance();
      Device->RenderData(LastChunk);
    }

  private:
    const AYM::DataIterator::Ptr Iterator;
    const Devices::AYM::Chip::Ptr Device;
    const Time::Duration<Devices::AYM::TimeUnit> FrameDuration;
    Devices::AYM::DataChunk LastChunk;
  };

  class AYMHolder : public AYM::Holder
  {
  public:
    AYMHolder(AYM::Chiptune::Ptr chiptune, Parameters::Accessor::Ptr properties)
      : Tune(std::move(chiptune))
      , Properties(std::move(properties))
    {}

    Information GetModuleInformation() const override
    {
      return ToInformation(Tune->GetInformation(), Tune->GetFrameDuration());
    }

    Parameters::Accessor::Ptr GetModuleProperties() const override
    {
      return Properties;
    }

    Renderer::Ptr CreateRenderer(uint_t samplerate, Parameters::Accessor::Ptr params) const override
    {
      auto chip = AYM::CreateChip(samplerate, params);
      auto trackParams = AYM::TrackParameters::Create(std::move(params));
      auto iterator = Tune->CreateDataIterator(std::move(trackParams));
      return MakePtr<AYMRenderer>(Tune->GetFrameDuration() /*TODO: speed variation*/, std::move(iterator),
                                  std::move(chip));
    }

    AYM::Chiptune::Ptr GetChiptune() const override
    {
      return Tune;
    }

    void Dump(Devices::AYM::Device& aym) const override
    {
      auto trackParams = AYM::TrackParameters::Create(Properties);
      const auto iterator = Tune->CreateDataIterator(std::move(trackParams));
      Devices::AYM::DataChunk chunk;
      for (const auto frameDuration = Tune->GetFrameDuration(); !iterator->GetState().LoopCount;
           chunk.TimeStamp += frameDuration)
      {
        chunk.Data = iterator->Advance();
        aym.RenderData(chunk);
      }
    }

  private:
    const AYM::Chiptune::Ptr Tune;
    const Parameters::Accessor::Ptr Properties;
  };
}  // namespace Module

namespace Module::AYM
{
  Holder::Ptr CreateHolder(Chiptune::Ptr chiptune, Parameters::Accessor::Ptr properties)
  {
    return MakePtr<AYMHolder>(std::move(chiptune), std::move(properties));
  }

  Devices::AYM::Chip::Ptr CreateChip(uint_t samplerate, Parameters::Accessor::Ptr params)
  {
    using MixerType = Sound::ThreeChannelsMatrixMixer;
    auto mixer = MixerType::Create();
    auto pollParams = Sound::CreateMixerNotificationParameters(std::move(params), mixer);
    auto chipParams = AYM::CreateChipParameters(samplerate, std::move(pollParams));
    return Devices::AYM::CreateChip(std::move(chipParams), std::move(mixer));
  }
}  // namespace Module::AYM
