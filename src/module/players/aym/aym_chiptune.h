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
#include "module/players/stream_model.h"
#include "module/players/track_model.h"
#include "module/players/aym/aym_parameters.h"
//library includes
#include <devices/aym.h>
#include <module/players/iterator.h>
#include <module/players/streaming.h>
#include <module/players/tracking.h>

namespace Module
{
  namespace AYM
  {
    class DataIterator : public StateIterator
    {
    public:
      typedef std::shared_ptr<DataIterator> Ptr;

      virtual Devices::AYM::Registers GetData() const = 0;
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
      virtual DataIterator::Ptr CreateDataIterator(TrackParameters::Ptr trackParams) const = 0;
    };
  }
}
