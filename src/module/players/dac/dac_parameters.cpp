/**
* 
* @file
*
* @brief  DAC-based parameters helpers implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "module/players/dac/dac_parameters.h"
//common includes
#include <make_ptr.h>
//library includes
#include <core/core_parameters.h>
#include <sound/render_params.h>

namespace Module
{
  namespace DAC
  {
    class ChipParameters : public Devices::DAC::ChipParameters
    {
    public:
      explicit ChipParameters(Parameters::Accessor::Ptr params)
        : Params(params)
        , SoundParams(Sound::RenderParameters::Create(std::move(params)))
      {
      }

      uint_t Version() const override
      {
        return Params->Version();
      }

      uint_t BaseSampleFreq() const override
      {
        Parameters::IntType intVal = 0;
        Params->FindValue(Parameters::ZXTune::Core::DAC::SAMPLES_FREQUENCY, intVal);
        return static_cast<uint_t>(intVal);
      }

      uint_t SoundFreq() const override
      {
        return SoundParams->SoundFreq();
      }

      bool Interpolate() const override
      {
        Parameters::IntType intVal = Parameters::ZXTune::Core::DAC::INTERPOLATION_DEFAULT;
        Params->FindValue(Parameters::ZXTune::Core::DAC::INTERPOLATION, intVal);
        return intVal != 0;
      }
    private:
      const Parameters::Accessor::Ptr Params;
      const Sound::RenderParameters::Ptr SoundParams;
    };

    Devices::DAC::ChipParameters::Ptr CreateChipParameters(Parameters::Accessor::Ptr params)
    {
      return MakePtr<ChipParameters>(std::move(params));
    }
  }
}
