/*
Abstract:
  TFM-based players common functionality

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef CORE_PLUGINS_PLAYERS_TFM_BASE_DEFINED
#define CORE_PLUGINS_PLAYERS_TFM_BASE_DEFINED

//local includes
#include "tfm_parameters.h"
#include "core/plugins/players/module_properties.h"
#include "core/plugins/players/tracking.h"
//library includes
#include <core/module_holder.h>

namespace ZXTune
{
  namespace Module
  {
    namespace TFM
    {
      class DataIterator : public StateIterator
      {
      public:
        typedef boost::shared_ptr<DataIterator> Ptr;

        virtual Devices::TFM::DataChunk GetData() const = 0;
      };

      class Chiptune
      {
      public:
        typedef boost::shared_ptr<const Chiptune> Ptr;
        virtual ~Chiptune() {}

        virtual Information::Ptr GetInformation() const = 0;
        virtual ModuleProperties::Ptr GetProperties() const = 0;
        virtual DataIterator::Ptr CreateDataIterator(TrackParameters::Ptr params) const = 0;

        virtual Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Devices::TFM::Device::Ptr chip) const;
      };

      Analyzer::Ptr CreateAnalyzer(Devices::TFM::Device::Ptr device);

      Renderer::Ptr CreateRenderer(TrackParameters::Ptr trackParams, DataIterator::Ptr iterator, Devices::TFM::Device::Ptr device);

      Devices::TFM::Receiver::Ptr CreateReceiver(Sound::OneChannelReceiver::Ptr target);

      Holder::Ptr CreateHolder(Chiptune::Ptr chiptune);
    }
  }
}

#endif //CORE_PLUGINS_PLAYERS_FM_BASE_DEFINED
