/**
* 
* @file
*
* @brief  AYM-based chiptunes support
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "aym_chiptune.h"
//library includes
#include <module/holder.h>
#include <sound/render_params.h>

namespace Module
{
  namespace AYM
  {
    class Holder : public Module::Holder
    {
    public:
      typedef boost::shared_ptr<const Holder> Ptr;

      using Module::Holder::CreateRenderer;
      virtual Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Devices::AYM::Device::Ptr chip) const = 0;
      virtual AYM::Chiptune::Ptr GetChiptune() const = 0;
    };

    Holder::Ptr CreateHolder(Chiptune::Ptr chiptune);

    Devices::AYM::Chip::Ptr CreateChip(Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target);
    Analyzer::Ptr CreateAnalyzer(Devices::AYM::Device::Ptr device);

    Renderer::Ptr CreateRenderer(Sound::RenderParameters::Ptr params, AYM::DataIterator::Ptr iterator, Devices::AYM::Device::Ptr device);
    Renderer::Ptr CreateRenderer(const Holder& holder, Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target);
  }
}
