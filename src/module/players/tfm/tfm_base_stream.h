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

#include "module/players/stream_model.h"
#include "module/players/tfm/tfm_chiptune.h"

namespace Module::TFM
{
  class StreamModel : public Module::StreamModel
  {
  public:
    using Ptr = std::shared_ptr<const StreamModel>;

    virtual void Get(uint_t pos, Devices::TFM::Registers& res) const = 0;
  };

  Chiptune::Ptr CreateStreamedChiptune(Time::Microseconds frameDuration, StreamModel::Ptr model,
                                       Parameters::Accessor::Ptr properties);
}  // namespace Module::TFM
