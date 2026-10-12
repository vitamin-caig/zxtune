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

#include "module/players/parameters.h"

#include "math/numeric.h"

#include "make_ptr.h"

#include <utility>

namespace Module
{
  class SAARenderer : public Renderer
  {
  public:
    SAARenderer(FrameDurationParameter frameDuration, SAA::DataIterator::Ptr iterator, Devices::SAA::Chip::Ptr device)
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
      LastChunk.TimeStamp += FrameDuration.Get<Devices::SAA::TimeUnit>();
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
    const SAA::DataIterator::Ptr Iterator;
    const Devices::SAA::Chip::Ptr Device;
    Devices::SAA::DataChunk LastChunk;
  };

  class SAAHolder : public Holder
  {
  public:
    SAAHolder(SAA::Chiptune::Ptr chiptune, Parameters::Accessor::Ptr properties, Time::Microseconds frameDuration)
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
      auto chipParams = SAA::CreateChipParameters(samplerate, std::move(params));
      auto chip = Devices::SAA::CreateChip(std::move(chipParams));
      auto iterator = Tune->CreateDataIterator();
      return MakePtr<SAARenderer>(FrameDurationParameter(std::move(rendererParams), SAA::BASE_FRAME_DURATION),
                                  std::move(iterator), std::move(chip));
    }

  private:
    const SAA::Chiptune::Ptr Tune;
    const Parameters::Accessor::Ptr Properties;
    const Time::Microseconds FrameDuration;
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

  Holder::Ptr CreateHolder(Chiptune::Ptr chiptune, Parameters::Accessor::Ptr properties)
  {
    const FrameDurationParameter frameDuration(properties, SAA::BASE_FRAME_DURATION);
    return MakePtr<SAAHolder>(std::move(chiptune), std::move(properties), frameDuration.Get());
  }
}  // namespace Module::SAA
