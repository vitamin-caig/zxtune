/**
* 
* @file
*
* @brief  AYM-based stream chiptunes support
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "module/players/aym/aym_chiptune.h"

namespace Module
{
  namespace AYM
  {
    class StreamModel
    {
    public:
      typedef std::shared_ptr<const StreamModel> Ptr;
      virtual ~StreamModel() = default;

      virtual uint_t Size() const = 0;
      virtual uint_t Loop() const = 0;
      virtual Devices::AYM::Registers Get(uint_t pos) const = 0;
    };

    Chiptune::Ptr CreateStreamedChiptune(StreamModel::Ptr model, Parameters::Accessor::Ptr properties);
  }
}
