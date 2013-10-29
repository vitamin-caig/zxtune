/**
* 
* @file
*
* @brief  TurboSound-based chiptunes support
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "aym_base_track.h"
//library includes
#include <core/module_holder.h>
#include <devices/turbosound.h>

namespace Module
{
  namespace TurboSound
  {
    const uint_t TRACK_CHANNELS = AYM::TRACK_CHANNELS * Devices::TurboSound::CHIPS;

    class DataIterator : public StateIterator
    {
    public:
      typedef boost::shared_ptr<DataIterator> Ptr;

      virtual Devices::TurboSound::Registers GetData() const = 0;
    };

    DataIterator::Ptr CreateDataIterator(AYM::TrackParameters::Ptr trackParams, TrackStateIterator::Ptr iterator,
      AYM::DataRenderer::Ptr first, AYM::DataRenderer::Ptr second);

    class Chiptune
    {
    public:
      typedef boost::shared_ptr<const Chiptune> Ptr;

      virtual Information::Ptr GetInformation() const = 0;
      virtual Parameters::Accessor::Ptr GetProperties() const = 0;
      virtual DataIterator::Ptr CreateDataIterator(AYM::TrackParameters::Ptr trackParams) const = 0;
    };

    Analyzer::Ptr CreateAnalyzer(Devices::TurboSound::Device::Ptr device);

    Chiptune::Ptr CreateChiptune(Parameters::Accessor::Ptr params, AYM::Chiptune::Ptr first, AYM::Chiptune::Ptr second);

    Renderer::Ptr CreateRenderer(Sound::RenderParameters::Ptr params, DataIterator::Ptr iterator, Devices::TurboSound::Device::Ptr device);
    Renderer::Ptr CreateRenderer(const Chiptune& chiptune, Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target);

    Holder::Ptr CreateHolder(Chiptune::Ptr chiptune);
  }
}
