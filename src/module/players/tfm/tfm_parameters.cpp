/**
 *
 * @file
 *
 * @brief  TFM parameters helpers implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/players/tfm/tfm_parameters.h"

#include "core/core_parameters.h"
#include "parameters/identifier.h"

#include "make_ptr.h"

#include <memory>
#include <utility>

namespace Module::TFM
{
  class ChipParameters : public Devices::TFM::ChipParameters
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

    uint64_t ClockFreq() const override
    {
      using namespace Parameters::ZXTune::Core::FM;
      return Parameters::GetInteger<uint64_t>(*Params, CLOCKRATE, CLOCKRATE_DEFAULT);
    }

    uint_t SoundFreq() const override
    {
      return Samplerate;
    }

  private:
    const uint_t Samplerate;
    const Parameters::Accessor::Ptr Params;
  };

  Devices::TFM::ChipParameters::Ptr CreateChipParameters(uint_t samplerate, Parameters::Accessor::Ptr params)
  {
    return MakePtr<ChipParameters>(samplerate, std::move(params));
  }
}  // namespace Module::TFM
