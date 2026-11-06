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

#include "module/players/aym/aym_base_track.h"
#include "module/players/parameters.h"
#include "module/players/streaming.h"
#include "module/players/tracking.h"

#include "debug/log.h"
#include "math/numeric.h"
#include "module/track_presentation.h"
#include "sound/mixer_factory.h"

#include "make_ptr.h"

namespace Module
{
  const Debug::Stream Dbg("Core::AYBase");

  class AYMRenderer : public Renderer
  {
  public:
    AYMRenderer(FrameDurationParameter frameDuration, AYM::DataIterator::Ptr iterator, Devices::AYM::Chip::Ptr device)
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
      LastChunk.TimeStamp += FrameDuration.Get<Devices::AYM::TimeUnit>();
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
    FrameDurationParameter FrameDuration;
    const AYM::DataIterator::Ptr Iterator;
    const Devices::AYM::Chip::Ptr Device;
    Devices::AYM::DataChunk LastChunk;
  };

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
      auto rendererParams = params;
      auto chip = AYM::CreateChip(samplerate, params);
      auto trackParams = AYM::TrackParameters::Create(std::move(params));
      auto iterator = Tune->CreateDataIterator(std::move(trackParams));
      return MakePtr<AYMRenderer>(FrameDurationParameter(std::move(rendererParams), AYM::BASE_FRAME_DURATION),
                                  std::move(iterator), std::move(chip));
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

  class AYMTrackHolder
    : public AYMHolder
    , public TrackPresentation
  {
  public:
    AYMTrackHolder(AYM::Chiptune::Ptr chiptune, std::unique_ptr<const TrackPresentation> presentation,
                   Parameters::Accessor::Ptr properties, Time::Microseconds frameDuration)
      : AYMHolder(std::move(chiptune), std::move(properties), frameDuration)
      , Presentation(std::move(presentation))
    {}

    String GetPattern(std::size_t index) const override
    {
      return Presentation->GetPattern(index);
    }

  private:
    const std::unique_ptr<const TrackPresentation> Presentation;
  };
}  // namespace Module

namespace Module::AYM
{
  Holder::Ptr CreateHolder(Chiptune::Ptr chiptune, Parameters::Accessor::Ptr properties)
  {
    const FrameDurationParameter frameDuration(properties, AYM::BASE_FRAME_DURATION);
    if (const auto* tracking = dynamic_cast<const TrackingChiptune*>(chiptune.get()))
    {
      if (auto presentation = tracking->CreatePresentation())
      {
        return MakePtr<AYMTrackHolder>(std::move(chiptune), std::move(presentation), std::move(properties),
                                       frameDuration.Get());
      }
    }
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
