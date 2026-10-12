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

#include "module/players/parameters.h"
#include "module/players/tfm/tfm_parameters.h"

#include "make_ptr.h"

#include <utility>

namespace Module
{
  class TFMRenderer : public Renderer
  {
  public:
    TFMRenderer(FrameDurationParameter frameDuration, TFM::DataIterator::Ptr iterator, Devices::TFM::Chip::Ptr device)
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
      LastChunk.TimeStamp += FrameDuration.Get<Devices::TFM::TimeUnit>();
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
    const TFM::DataIterator::Ptr Iterator;
    const Devices::TFM::Chip::Ptr Device;
    Devices::TFM::DataChunk LastChunk;
  };

  class TFMHolder : public Holder
  {
  public:
    TFMHolder(TFM::Chiptune::Ptr chiptune, Parameters::Accessor::Ptr properties, Time::Microseconds frameDuration)
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
      auto chipParams = TFM::CreateChipParameters(samplerate, std::move(params));
      auto chip = Devices::TFM::CreateChip(std::move(chipParams));
      auto iterator = Tune->CreateDataIterator();
      return TFM::CreateRenderer(FrameDurationParameter(std::move(rendererParams), TFM::BASE_FRAME_DURATION),
                                 std::move(iterator), std::move(chip));
    }

  private:
    const TFM::Chiptune::Ptr Tune;
    const Parameters::Accessor::Ptr Properties;
    const Time::Microseconds FrameDuration;
  };
}  // namespace Module

namespace Module::TFM
{
  Renderer::Ptr CreateRenderer(FrameDurationParameter frameDuration, DataIterator::Ptr iterator,
                               Devices::TFM::Chip::Ptr device)
  {
    return MakePtr<TFMRenderer>(std::move(frameDuration), std::move(iterator), std::move(device));
  }

  Holder::Ptr CreateHolder(Chiptune::Ptr chiptune, Parameters::Accessor::Ptr properties)
  {
    const FrameDurationParameter frameDuration(properties, TFM::BASE_FRAME_DURATION);
    return MakePtr<TFMHolder>(std::move(chiptune), std::move(properties), frameDuration.Get());
  }
}  // namespace Module::TFM
