/**
 *
 * @file
 *
 * @brief  SAA parameters helpers implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/players/saa/saa_parameters.h"

#include "core/parameters.h"

#include "make_ptr.h"

#include <utility>

namespace Module::SAA
{
  class ChipParameters : public Devices::SAA::ChipParameters
  {
  public:
    ChipParameters(uint_t samplerate, Parameters::Accessor::Ptr params)
      : Samplerate(samplerate)
      , Params(std::move(params))
    {}

    uint_t Version() const override
    {
      return Params->Version();
    }

    uint_t ClockFreq() const override
    {
      using namespace Parameters::ZXTune::Core::SAA;
      return Parameters::Get<Clockrate>(*Params);
    }

    uint_t SoundFreq() const override
    {
      return Samplerate;
    }

    Devices::SAA::InterpolationType Interpolation() const override
    {
      return static_cast<Devices::SAA::InterpolationType>(
          Parameters::Get<Parameters::ZXTune::Core::SAA::Interpolation>(*Params));
    }

    uint_t MuteMask() const override
    {
      using namespace Parameters::ZXTune::Core;
      return Parameters::Get<ChannelsMask>(*Params);
    }

  private:
    const uint_t Samplerate;
    const Parameters::Accessor::Ptr Params;
  };

  Devices::SAA::ChipParameters::Ptr CreateChipParameters(uint_t samplerate, Parameters::Accessor::Ptr params)
  {
    return MakePtr<ChipParameters>(samplerate, std::move(params));
  }
}  // namespace Module::SAA
