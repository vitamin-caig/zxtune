/**
 *
 * @file
 *
 * @brief  AYM-based chiptunes common functionality implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "module/players/aym/aym_base.h"
#include "module/players/streaming.h"
#include "module/players/tracking.h"
// common includes
#include <make_ptr.h>
// library includes
#include <debug/log.h>
#include <math/numeric.h>
#include <module/loop.h>
#include <sound/mixer_factory.h>

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

    State::Ptr GetState() const override
    {
      return Iterator->GetStateObserver();
    }

    Sound::Chunk Render(const LoopParameters& looped) override
    {
      if (!looped(GetState()->LoopCount()))
      {
        return {};
      }
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
      while (state->At() < request)
      {
        TransferChunk();
        Iterator->NextFrame();
      }
    }

  private:
    void TransferChunk()
    {
      LastChunk.Data = Iterator->GetData();
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
    AYMHolder(AYM::Chiptune::Ptr chiptune)
      : Tune(std::move(chiptune))
    {}

    Information::Ptr GetModuleInformation() const override
    {
      if (auto track = Tune->FindTrackModel())
      {
        return CreateTrackInfo(Tune->GetFrameDuration(), std::move(track));
      }
      else
      {
        return CreateStreamInfo(Tune->GetFrameDuration(), Tune->FindStreamModel());
      }
    }

    Parameters::Accessor::Ptr GetModuleProperties() const override
    {
      return Tune->GetProperties();
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
      auto trackParams = AYM::TrackParameters::Create(Tune->GetProperties());
      const auto iterator = Tune->CreateDataIterator(std::move(trackParams));
      const auto state = iterator->GetStateObserver();
      Devices::AYM::DataChunk chunk;
      for (const auto frameDuration = Tune->GetFrameDuration(); !state->LoopCount();
           chunk.TimeStamp += frameDuration, iterator->NextFrame())
      {
        chunk.Data = iterator->GetData();
        aym.RenderData(chunk);
      }
    }

  private:
    const AYM::Chiptune::Ptr Tune;
  };
}  // namespace Module

namespace Module
{
  namespace AYM
  {
    Holder::Ptr CreateHolder(Chiptune::Ptr chiptune)
    {
      return MakePtr<AYMHolder>(std::move(chiptune));
    }

    Devices::AYM::Chip::Ptr CreateChip(uint_t samplerate, Parameters::Accessor::Ptr params)
    {
      typedef Sound::ThreeChannelsMatrixMixer MixerType;
      auto mixer = MixerType::Create();
      auto pollParams = Sound::CreateMixerNotificationParameters(std::move(params), mixer);
      auto chipParams = AYM::CreateChipParameters(samplerate, std::move(pollParams));
      return Devices::AYM::CreateChip(std::move(chipParams), std::move(mixer));
    }
  }  // namespace AYM
}  // namespace Module
