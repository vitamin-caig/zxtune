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
#include "module/players/aym/aym_base_track.h"
//library includes
#include <devices/turbosound.h>
#include <module/holder.h>
#include <sound/render_params.h>

namespace Module
{
  namespace TurboSound
  {
    const uint_t TRACK_CHANNELS = AYM::TRACK_CHANNELS * Devices::TurboSound::CHIPS;

    class DataIterator : public StateIterator
    {
    public:
      typedef std::shared_ptr<DataIterator> Ptr;

      virtual Devices::TurboSound::Registers GetData() const = 0;
    };

    class Chiptune
    {
    public:
      typedef std::shared_ptr<const Chiptune> Ptr;
      virtual ~Chiptune() = default;

      // One of
      virtual TrackModel::Ptr FindTrackModel() const = 0;
      virtual Module::StreamModel::Ptr FindStreamModel() const = 0;

      virtual Parameters::Accessor::Ptr GetProperties() const = 0;
      virtual DataIterator::Ptr CreateDataIterator(AYM::TrackParameters::Ptr first, AYM::TrackParameters::Ptr second) const = 0;
    };

    Analyzer::Ptr CreateAnalyzer(Devices::TurboSound::Device::Ptr device);

    Chiptune::Ptr CreateChiptune(Parameters::Accessor::Ptr params, AYM::Chiptune::Ptr first, AYM::Chiptune::Ptr second);

    Holder::Ptr CreateHolder(Chiptune::Ptr chiptune);
  }
}
