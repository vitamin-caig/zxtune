/**
 *
 * @file
 *
 * @brief  SAA-based chiptunes common functionality implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/players/saa/saa_base.h"

#include "module/players/saa/saa_parameters.h"

#include "math/numeric.h"
#include "module/information.h"
#include "module/renderer.h"
#include "sound/chunk.h"
#include "time/instant.h"

#include "make_ptr.h"

#include <utility>

namespace Module
{
  class SAADataIterator : public SAA::DataIterator
  {
  public:
    SAADataIterator(TrackStateIterator::Ptr delegate, SAA::DataRenderer::Ptr renderer)
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

    Devices::SAA::Registers GetData() const override
    {
      return CurrentData;
    }

  private:
    void FillCurrentData()
    {
      SAA::TrackBuilder builder;
      Render->SynthesizeData(*State, builder);
      builder.GetResult(CurrentData);
    }

  private:
    const TrackStateIterator::Ptr Delegate;
    const TrackModelState::Ptr State;
    const SAA::DataRenderer::Ptr Render;
    Devices::SAA::Registers CurrentData;
  };

  class SAARenderer : public Renderer
  {
  public:
    SAARenderer(Time::Microseconds frameDuration, SAA::DataIterator::Ptr iterator, Devices::SAA::Chip::Ptr device)
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
    const SAA::DataIterator::Ptr Iterator;
    const Devices::SAA::Chip::Ptr Device;
    const Time::Duration<Devices::SAA::TimeUnit> FrameDuration;
    Devices::SAA::DataChunk LastChunk;
  };

  class SAAHolder : public Holder
  {
  public:
    explicit SAAHolder(SAA::Chiptune::Ptr chiptune)
      : Tune(std::move(chiptune))
    {}

    Information::Ptr GetModuleInformation() const override
    {
      return CreateTrackInfo(Tune->GetFrameDuration(), Tune->GetTrackModel());
    }

    Parameters::Accessor::Ptr GetModuleProperties() const override
    {
      return Tune->GetProperties();
    }

    Renderer::Ptr CreateRenderer(uint_t samplerate, Parameters::Accessor::Ptr params) const override
    {
      auto chipParams = SAA::CreateChipParameters(samplerate, std::move(params));
      auto chip = Devices::SAA::CreateChip(std::move(chipParams));
      auto iterator = Tune->CreateDataIterator();
      return MakePtr<SAARenderer>(Tune->GetFrameDuration() /*TODO: playback speed*/, std::move(iterator),
                                  std::move(chip));
    }

  private:
    const SAA::Chiptune::Ptr Tune;
  };
}  // namespace Module

namespace Module::SAA
{
  ChannelBuilder::ChannelBuilder(uint_t chan, Devices::SAA::Registers& regs)
    : Channel(chan)
    , Regs(regs)
  {
    SetRegister(Devices::SAA::Registers::TONEMIXER, 0, 1 << chan);
    SetRegister(Devices::SAA::Registers::NOISEMIXER, 0, 1 << chan);
  }

  void ChannelBuilder::SetVolume(int_t left, int_t right)
  {
    SetRegister(Devices::SAA::Registers::LEVEL0 + Channel,
                16 * Math::Clamp<int_t>(right, 0, 15) + Math::Clamp<int_t>(left, 0, 15));
  }

  void ChannelBuilder::SetTone(uint_t octave, uint_t note)
  {
    SetRegister(Devices::SAA::Registers::TONENUMBER0 + Channel, note);
    AddRegister(Devices::SAA::Registers::TONEOCTAVE01 + Channel / 2, 0 != (Channel & 1) ? (octave << 4) : octave);
  }

  void ChannelBuilder::SetNoise(uint_t type)
  {
    const uint_t shift = Channel >= 3 ? 4 : 0;
    SetRegister(Devices::SAA::Registers::NOISECLOCK, type << shift, 0x7 << shift);
  }

  void ChannelBuilder::AddNoise(uint_t type)
  {
    const uint_t shift = Channel >= 3 ? 4 : 0;
    AddRegister(Devices::SAA::Registers::NOISECLOCK, type << shift);
  }

  void ChannelBuilder::SetEnvelope(uint_t type)
  {
    SetRegister(Devices::SAA::Registers::ENVELOPE0 + (Channel >= 3), type);
  }

  void ChannelBuilder::EnableTone()
  {
    AddRegister(Devices::SAA::Registers::TONEMIXER, 1 << Channel);
  }

  void ChannelBuilder::EnableNoise()
  {
    AddRegister(Devices::SAA::Registers::NOISEMIXER, 1 << Channel);
  }

  DataIterator::Ptr CreateDataIterator(TrackStateIterator::Ptr iterator, DataRenderer::Ptr renderer)
  {
    return MakePtr<SAADataIterator>(std::move(iterator), std::move(renderer));
  }

  Holder::Ptr CreateHolder(Chiptune::Ptr chiptune)
  {
    return MakePtr<SAAHolder>(std::move(chiptune));
  }
}  // namespace Module::SAA
