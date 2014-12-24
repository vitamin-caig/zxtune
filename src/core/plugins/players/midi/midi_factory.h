/**
* 
* @file
*
* @brief  MIDI-based modules factory
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "midi_chiptune.h"
#include "core/plugins/players/module_properties.h"
//library includes
#include <binary/container.h>

namespace Module
{
  namespace MIDI
  {
    class Factory
    {
    public:
      typedef boost::shared_ptr<const Factory> Ptr;
      virtual ~Factory() {}

      virtual Chiptune::Ptr CreateChiptune(PropertiesBuilder& properties, const Binary::Container& data) const = 0;
    };
  }
}
