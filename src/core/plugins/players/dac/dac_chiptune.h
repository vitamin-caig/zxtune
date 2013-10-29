/**
* 
* @file
*
* @brief  DAC-based chiptune interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "core/plugins/players/tracking.h"
//library includes
#include <devices/dac.h>
#include <parameters/accessor.h>

namespace Module
{
  namespace DAC
  {
    class DataIterator : public StateIterator
    {
    public:
      typedef boost::shared_ptr<DataIterator> Ptr;

      virtual void GetData(Devices::DAC::Channels& data) const = 0;
    };

    class Chiptune
    {
    public:
      typedef boost::shared_ptr<const Chiptune> Ptr;
      virtual ~Chiptune() {}

      virtual Information::Ptr GetInformation() const = 0;
      virtual Parameters::Accessor::Ptr GetProperties() const = 0;
      virtual DataIterator::Ptr CreateDataIterator() const = 0;
      virtual void GetSamples(Devices::DAC::Chip::Ptr chip) const = 0;
    };
  }
}
