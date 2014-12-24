/**
* 
* @file
*
* @brief  MIDI parameters helpers implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "midi_parameters.h"
//library includes
#include <core/core_parameters.h>
#include <sound/render_params.h>
//boost includes
#include <boost/make_shared.hpp>

namespace Module
{
namespace MIDI
{
  class ChipParameters : public Devices::MIDI::ChipParameters
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

    virtual uint_t SoundFreq() const
    {
      return SoundParams->SoundFreq();
    }
  private:
    const Parameters::Accessor::Ptr Params;
    const Sound::RenderParameters::Ptr SoundParams;
  };

  Devices::MIDI::ChipParameters::Ptr CreateChipParameters(Parameters::Accessor::Ptr params)
  {
    return boost::make_shared<ChipParameters>(params);
  }
}
}
