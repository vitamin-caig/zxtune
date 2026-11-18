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

#include "module/players/framed_renderer.h"
#include "module/players/parameters.h"
#include "module/players/streaming.h"
#include "module/players/tracking.h"

#include "debug/log.h"
#include "math/numeric.h"
#include "sound/mixer_factory.h"

#include "make_ptr.h"

namespace Module
{
  const Debug::Stream Dbg("Core::AYBase");

  class AYMHolder : public AYM::Holder
  {
  public:
    AYMHolder(AYM::Chiptune::Ptr chiptune, Parameters::Accessor::Ptr properties, Time::Microseconds frameDuration)
      : Tune(std::move(chiptune))
      , Properties(std::move(properties))
      , FrameDuration(frameDuration)
    {}

    Information GetModuleInformation() const override
    {
      return ToInformation(Tune->GetInformation(), FrameDuration);
    }

    Parameters::Accessor::Ptr GetModuleProperties() const override
    {
      return Properties;
    }

    Renderer::Ptr CreateRenderer(uint_t samplerate, Parameters::Accessor::Ptr params) const override
    {
      auto chip = AYM::CreateChip(samplerate, params);
      auto trackParams = AYM::TrackParameters::Create(params);
      auto iterator = Tune->CreateDataIterator(std::move(trackParams));
      return CreateFramedRenderer(std::move(params), AYM::BASE_FRAME_DURATION, std::move(iterator), std::move(chip));
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
      for (const auto frameDuration = FrameDuration; !iterator->GetState().LoopCount; chunk.TimeStamp += frameDuration)
      {
        chunk.Data = iterator->Advance();
        aym.RenderData(chunk);
      }
    }

  private:
    const AYM::Chiptune::Ptr Tune;
    const Parameters::Accessor::Ptr Properties;
    const Time::Microseconds FrameDuration;
  };
}  // namespace Module

namespace Module::AYM
{
  Holder::Ptr CreateHolder(Chiptune::Ptr chiptune, Parameters::Accessor::Ptr properties)
  {
    const FrameDurationParameter frameDuration(properties, AYM::BASE_FRAME_DURATION);
    return MakePtr<AYMHolder>(std::move(chiptune), std::move(properties), frameDuration.Get());
  }

  Devices::AYM::Chip::Ptr CreateChip(uint_t samplerate, Parameters::Accessor::Ptr params)
  {
    using MixerType = Sound::ThreeChannelsMatrixMixer;
    auto mixer = MixerType::Create();
    auto pollParams = Sound::CreateMixerNotificationParameters(std::move(params), mixer);
    auto chipParams = AYM::ChipParameters::Create(samplerate, std::move(pollParams));
    return Devices::AYM::CreateChip(std::move(chipParams), std::move(mixer));
  }
}  // namespace Module::AYM
