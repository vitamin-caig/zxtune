/**
 *
 * @file
 *
 * @brief  TFM-based chiptunes common functionality implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/players/tfm/tfm_base.h"

#include "module/players/tfm/tfm_parameters.h"

#include "make_ptr.h"

#include <utility>

namespace Module
{
  class TFMRenderer : public Renderer
  {
  public:
    TFMRenderer(Time::Microseconds frameDuration, TFM::DataIterator::Ptr iterator, Devices::TFM::Chip::Ptr device)
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
    const TFM::DataIterator::Ptr Iterator;
    const Devices::TFM::Chip::Ptr Device;
    const Time::Duration<Devices::TFM::TimeUnit> FrameDuration;
    Devices::TFM::DataChunk LastChunk;
  };

  class TFMHolder : public Holder
  {
  public:
    TFMHolder(TFM::Chiptune::Ptr chiptune, Parameters::Accessor::Ptr properties)
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
      auto chipParams = TFM::ChipParameters::Create(samplerate, std::move(params));
      auto chip = Devices::TFM::CreateChip(std::move(chipParams));
      auto iterator = Tune->CreateDataIterator();
      return TFM::CreateRenderer(Tune->GetFrameDuration() /*TODO: speed variation*/, std::move(iterator),
                                 std::move(chip));
    }

  private:
    const TFM::Chiptune::Ptr Tune;
    const Parameters::Accessor::Ptr Properties;
  };
}  // namespace Module

namespace Module::TFM
{
  Renderer::Ptr CreateRenderer(Time::Microseconds frameDuration, TFM::DataIterator::Ptr iterator,
                               Devices::TFM::Chip::Ptr device)
  {
    return MakePtr<TFMRenderer>(frameDuration, std::move(iterator), std::move(device));
  }

  Holder::Ptr CreateHolder(Chiptune::Ptr chiptune, Parameters::Accessor::Ptr properties)
  {
    return MakePtr<TFMHolder>(std::move(chiptune), std::move(properties));
  }
}  // namespace Module::TFM
