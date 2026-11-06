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

#include "module/players/framed_renderer.h"
#include "module/players/parameters.h"
#include "module/players/tfm/tfm_parameters.h"

#include "module/track_presentation.h"

#include "make_ptr.h"

#include <memory>
#include <utility>

namespace Module
{
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
      auto chipParams = TFM::ChipParameters::Create(samplerate, params);
      auto chip = Devices::TFM::CreateChip(std::move(chipParams));
      auto iterator = Tune->CreateDataIterator();
      return CreateFramedRenderer(std::move(params), TFM::BASE_FRAME_DURATION, std::move(iterator), std::move(chip));
    }

  private:
    const TFM::Chiptune::Ptr Tune;
    const Parameters::Accessor::Ptr Properties;
    const Time::Microseconds FrameDuration;
  };

  class TFMTrackHolder
    : public TFMHolder
    , public TrackPresentation
  {
  public:
    using Ptr = Module::Holder::Ptr;

    TFMTrackHolder(TFM::Chiptune::Ptr chiptune, std::unique_ptr<const TrackPresentation> presentation,
                   Parameters::Accessor::Ptr properties, Time::Microseconds frameDuration)
      : TFMHolder(std::move(chiptune), std::move(properties), frameDuration)
      , Presentation(std::move(presentation))
    {}

    String GetPattern(uint_t index) const override
    {
      return Presentation->GetPattern(index);
    }

  private:
    const std::unique_ptr<const TrackPresentation> Presentation;
  };
}  // namespace Module

namespace Module::TFM
{
  Holder::Ptr CreateHolder(Chiptune::Ptr chiptune, Parameters::Accessor::Ptr properties)
  {
    const FrameDurationParameter frameDuration(properties, TFM::BASE_FRAME_DURATION);
    if (const auto* tracking = dynamic_cast<const TrackingChiptune*>(chiptune.get()))
    {
      if (auto presentation = tracking->CreatePresentation())
      {
        return MakePtr<TFMTrackHolder>(std::move(chiptune), std::move(presentation), std::move(properties),
                                       frameDuration.Get());
      }
    }
    return MakePtr<TFMHolder>(std::move(chiptune), std::move(properties), frameDuration.Get());
  }
}  // namespace Module::TFM
