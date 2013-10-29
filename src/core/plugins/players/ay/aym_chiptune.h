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
#include "aym_parameters.h"
#include "core/plugins/players/tracking.h"
//library includes
#include <devices/aym.h>

namespace Module
{
  namespace AYM
  {
    class DataIterator : public StateIterator
    {
    public:
      typedef boost::shared_ptr<DataIterator> Ptr;

      virtual Devices::AYM::Registers GetData() const = 0;
    };

    class Chiptune
    {
    public:
      typedef boost::shared_ptr<const Chiptune> Ptr;
      virtual ~Chiptune() {}

      virtual Information::Ptr GetInformation() const = 0;
      virtual Parameters::Accessor::Ptr GetProperties() const = 0;
      virtual DataIterator::Ptr CreateDataIterator(TrackParameters::Ptr trackParams) const = 0;
    };
  }
}
