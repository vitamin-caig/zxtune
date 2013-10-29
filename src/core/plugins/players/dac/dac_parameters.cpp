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
#include "dac_parameters.h"
//library includes
#include <core/core_parameters.h>
#include <sound/render_params.h>
//boost includes
#include <boost/make_shared.hpp>

namespace Module
{
  namespace DAC
  {
    class ChipParameters : public Devices::DAC::ChipParameters
    {
    public:
      explicit ChipParameters(Parameters::Accessor::Ptr params)
        : Params(params)
        , SoundParams(Sound::RenderParameters::Create(params))
      {
      }

      virtual uint_t Version() const
      {
        return Params->Version();
      }

      virtual uint_t BaseSampleFreq() const
      {
        Parameters::IntType intVal = 0;
        Params->FindValue(Parameters::ZXTune::Core::DAC::SAMPLES_FREQUENCY, intVal);
        return static_cast<uint_t>(intVal);
      }

      virtual uint_t SoundFreq() const
      {
        return SoundParams->SoundFreq();
      }

      virtual bool Interpolate() const
      {
        Parameters::IntType intVal = 0;
        Params->FindValue(Parameters::ZXTune::Core::DAC::INTERPOLATION, intVal);
        return intVal != 0;
      }
    private:
      const Parameters::Accessor::Ptr Params;
      const Sound::RenderParameters::Ptr SoundParams;
    };

    Devices::DAC::ChipParameters::Ptr CreateChipParameters(Parameters::Accessor::Ptr params)
    {
      return boost::make_shared<ChipParameters>(params);
    }
  }
}
