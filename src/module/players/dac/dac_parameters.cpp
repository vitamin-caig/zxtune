/**
 *
 * @file
 *
 * @brief  DAC-based parameters helpers implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/players/dac/dac_parameters.h"

#include "core/core_parameters.h"

#include "make_ptr.h"

#include <utility>

namespace Module::DAC
{
  class ChipParameters : public Devices::DAC::ChipParameters
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

    uint_t BaseSampleFreq() const override
    {
      return Parameters::GetInteger<uint_t>(*Params, Parameters::ZXTune::Core::DAC::SAMPLES_FREQUENCY);
    }

    uint_t SoundFreq() const override
    {
      return Samplerate;
    }

    bool Interpolate() const override
    {
      using namespace Parameters::ZXTune::Core::DAC;
      return 0 != Parameters::GetInteger(*Params, INTERPOLATION, INTERPOLATION_DEFAULT);
    }

    uint_t MuteMask() const override
    {
      using namespace Parameters::ZXTune::Core;
      return Parameters::GetInteger(*Params, CHANNELS_MASK, CHANNELS_MASK_DEFAULT);
    }

  private:
    const uint_t Samplerate;
    const Parameters::Accessor::Ptr Params;
  };

  Devices::DAC::ChipParameters::Ptr CreateChipParameters(uint_t samplerate, Parameters::Accessor::Ptr params)
  {
    return MakePtr<ChipParameters>(samplerate, std::move(params));
  }
}  // namespace Module::DAC
