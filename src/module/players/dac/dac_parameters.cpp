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

#include "core/parameters.h"

#include "make_ptr.h"

#include <utility>

namespace Module::DAC
{
  namespace Dac = Parameters::ZXTune::Core::DAC;

  class ChipParametersImpl : public Devices::DAC::ChipParameters
  {
  public:
    ChipParametersImpl(uint_t samplerate, Parameters::Accessor::Ptr params)
      : Samplerate(samplerate)
      , Params(std::move(params))
    {}

    uint_t Version() const override
    {
      return Params->Version();
    }

    uint_t BaseSampleFreq() const override
    {
      return static_cast<uint_t>(
          Parameters::Find<Parameters::ZXTune::Core::DAC::SamplesFrequency>(*Params).value_or(0));
    }

    uint_t SoundFreq() const override
    {
      return Samplerate;
    }

    bool Interpolate() const override
    {
      using namespace Parameters::ZXTune::Core::DAC;
      return Parameters::Get<Interpolation>(*Params);
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

  const Parameters::IdentifiersSet ChipParameters::ALL{Dac::Interpolation::Key, Dac::SamplesFrequency::Key};

  Devices::DAC::ChipParameters::Ptr ChipParameters::Create(uint_t samplerate, Parameters::Accessor::Ptr params)
  {
    return MakePtr<ChipParametersImpl>(samplerate, std::move(params));
  }
}  // namespace Module::DAC
