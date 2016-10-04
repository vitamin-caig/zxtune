/**
* 
* @file
*
* @brief  TFM-based stream chiptunes support
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "tfm_chiptune.h"

namespace Module
{
  namespace TFM
  {
    class StreamModel
    {
    public:
      typedef std::shared_ptr<const StreamModel> Ptr;
      virtual ~StreamModel() {}

      virtual uint_t Size() const = 0;
      virtual uint_t Loop() const = 0;
      virtual void Get(uint_t pos, Devices::TFM::Registers& res) const = 0;
    };

    Chiptune::Ptr CreateStreamedChiptune(StreamModel::Ptr model, Parameters::Accessor::Ptr properties);
  }
}
