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

#include "module/players/tfm/tfm_chiptune.h"

namespace Module::TFM
{
  class StreamModel
  {
  public:
    using Ptr = std::shared_ptr<const StreamModel>;

    virtual FramedInformation GetInformation() const = 0;
    virtual void Get(uint_t pos, Devices::TFM::Registers& res) const = 0;
  };

  Chiptune::Ptr CreateStreamedChiptune(Time::Microseconds frameDuration, StreamModel::Ptr model);
  Chiptune::Ptr CreateStreamedChiptune(StreamModel::Ptr model);
}  // namespace Module::TFM
