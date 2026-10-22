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

#include "core/parameters.h"
#include "l10n/api.h"

#include "error_tools.h"
#include "make_ptr.h"

#include <utility>

namespace Module::TFM
{
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("module");

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

    uint_t ClockFreq() const override
    {
      using namespace Parameters::ZXTune::Core::FM;
      const auto val = Parameters::Get<Clockrate>(*Params);
      if (!Parameters::Validate<Clockrate>(val))
      {
        throw MakeFormattedError(THIS_LINE, translate("Invalid clock frequency ({})."), val);
      }
      return val;
    }

    uint_t SoundFreq() const override
    {
      return Samplerate;
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

  Devices::TFM::ChipParameters::Ptr CreateChipParameters(uint_t samplerate, Parameters::Accessor::Ptr params)
  {
    return MakePtr<ChipParameters>(samplerate, std::move(params));
  }
}  // namespace Module::TFM
