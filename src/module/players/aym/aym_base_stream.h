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

// local includes
#include "module/players/aym/aym_chiptune.h"
#include "module/players/stream_model.h"
// common includes
#include <contract.h>
// library includes

namespace Module::AYM
{
  class StreamModel : public Module::StreamModel
  {
  public:
    using Ptr = std::shared_ptr<const StreamModel>;

    uint_t GetTotalFrames() const override
    {
      return static_cast<uint_t>(Data.size());
    }

    uint_t GetLoopFrame() const override
    {
      return Loop;
    }

    const Devices::AYM::Registers& Get(uint_t pos) const
    {
      return Data[pos];
    }

  protected:
    uint_t Loop = 0;
    std::vector<Devices::AYM::Registers> Data;
  };

  class MutableStreamModel : public StreamModel
  {
  public:
    using Ptr = std::shared_ptr<MutableStreamModel>;

    bool IsEmpty() const
    {
      return Data.empty();
    }

    void SetLoop(uint_t frame)
    {
      Loop = frame;
    }

    void Resize(std::size_t size)
    {
      Require(Data.empty());
      Data.resize(size);
    }

    void Append(std::size_t count)
    {
      Data.resize(Data.size() + count);
    }

    Devices::AYM::Registers& Frame(uint_t pos)
    {
      return Data.at(pos);
    }

    Devices::AYM::Registers* LastFrame()
    {
      return Data.empty() ? nullptr : &Data.back();
    }

    Devices::AYM::Registers& AddFrame()
    {
      Data.push_back({});
      return Data.back();
    }
  };

  Chiptune::Ptr CreateStreamedChiptune(Time::Microseconds frameDuration, StreamModel::Ptr model,
                                       Parameters::Accessor::Ptr properties);
}  // namespace Module::AYM
