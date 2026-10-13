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

#include "module/players/aym/aym_chiptune.h"

#include "contract.h"

namespace Module::AYM
{
  class StreamModel
  {
  public:
    using Ptr = std::shared_ptr<const StreamModel>;
    using Data = Devices::AYM::Registers;

    FramedInformation GetInformation() const
    {
      return {.Total = static_cast<uint_t>(Frames.size()), .Loop = Loop};
    }

    const Devices::AYM::Registers& Get(uint_t pos) const
    {
      return Frames[pos];
    }

  protected:
    uint_t Loop = 0;
    std::vector<Devices::AYM::Registers> Frames;
  };

  class MutableStreamModel : public StreamModel
  {
  public:
    using Ptr = std::shared_ptr<MutableStreamModel>;

    bool IsEmpty() const
    {
      return Frames.empty();
    }

    void SetLoop(uint_t frame)
    {
      Loop = frame;
    }

    void Resize(std::size_t size)
    {
      Require(Frames.empty());
      Frames.resize(size);
    }

    void Append(std::size_t count)
    {
      Frames.resize(Frames.size() + count);
    }

    Devices::AYM::Registers& Frame(uint_t pos)
    {
      return Frames.at(pos);
    }

    Devices::AYM::Registers* LastFrame()
    {
      return Frames.empty() ? nullptr : &Frames.back();
    }

    Devices::AYM::Registers& AddFrame()
    {
      Frames.emplace_back();
      return Frames.back();
    }
  };

  Chiptune::Ptr CreateStreamedChiptune(Time::Microseconds frameDuration, StreamModel::Ptr model);
  Chiptune::Ptr CreateStreamedChiptune(StreamModel::Ptr model);
}  // namespace Module::AYM
