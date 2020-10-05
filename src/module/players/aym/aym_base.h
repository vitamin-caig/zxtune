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
#include "module/players/aym/aym_chiptune.h"
//library includes
#include <module/holder.h>

namespace Module
{
  namespace AYM
  {
    class Holder : public Module::Holder
    {
    public:
      typedef std::shared_ptr<const Holder> Ptr;

      using Module::Holder::CreateRenderer;
      virtual Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Devices::AYM::Device::Ptr chip) const = 0;
      virtual AYM::Chiptune::Ptr GetChiptune() const = 0;
    };

    Holder::Ptr CreateHolder(Time::Microseconds frameDuration, Chiptune::Ptr chiptune);

    Devices::AYM::Chip::Ptr CreateChip(Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target);
    Analyzer::Ptr CreateAnalyzer(Devices::AYM::Device::Ptr device);

    Renderer::Ptr CreateRenderer(const Parameters::Accessor& params, AYM::DataIterator::Ptr iterator, Devices::AYM::Device::Ptr device);
    Renderer::Ptr CreateRenderer(const Holder& holder, Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target);
  }
}
