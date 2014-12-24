/**
* 
* @file
*
* @brief  MIDI-based stream chiptunes support
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "midi_chiptune.h"

namespace Module
{
  namespace MIDI
  {
    class StreamModel
    {
    public:
      typedef boost::shared_ptr<const StreamModel> Ptr;
      virtual ~StreamModel() {}

      virtual uint_t Size() const = 0;
      virtual uint_t Loop() const = 0;
      virtual void Get(uint_t pos, Devices::MIDI::DataChunk& res) const = 0;
    };

    Chiptune::Ptr CreateStreamedChiptune(StreamModel::Ptr model, Parameters::Accessor::Ptr properties);
  }
}
