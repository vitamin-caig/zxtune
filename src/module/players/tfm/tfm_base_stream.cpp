/**
 *
 * @file
 *
 * @brief  TFM-based stream chiptunes support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/players/tfm/tfm_base_stream.h"

#include "module/players/streaming.h"

#include "make_ptr.h"

#include <utility>

namespace Module::TFM
{
  class StreamDataIterator : public DataIterator
  {
  public:
    StreamDataIterator(Iterator::Ptr delegate, StreamModel::Ptr data)
      : Delegate(std::move(delegate))
      , Data(std::move(data))
    {}

    void Reset() override
    {
      Delegate->Reset();
    }

    void NextFrame() override
    {
      Delegate->NextFrame();
    }

    FramedState GetState() const override
    {
      return Delegate->GetState();
    }

    void GetData(Devices::TFM::Registers& res) const override
    {
      Data->Get(Delegate->GetState().At, res);
    }

  private:
    const Iterator::Ptr Delegate;
    const StreamModel::Ptr Data;
  };

  class StreamedChiptune : public Chiptune
  {
  public:
    StreamedChiptune(Time::Microseconds frameDuration, StreamModel::Ptr model)
      : FrameDuration(frameDuration)
      , Data(std::move(model))
    {}

    Time::Microseconds GetFrameDuration() const override
    {
      return FrameDuration;
    }

    FramedInformation GetInformation() const override
    {
      return Data->GetInformation();
    }

    TFM::DataIterator::Ptr CreateDataIterator() const override
    {
      auto iter = CreateStreamStateIterator(Data->GetInformation());
      return MakePtr<StreamDataIterator>(std::move(iter), Data);
    }

  private:
    const Time::Microseconds FrameDuration;
    const StreamModel::Ptr Data;
  };

  Chiptune::Ptr CreateStreamedChiptune(Time::Microseconds frameDuration, StreamModel::Ptr model)
  {
    return MakePtr<StreamedChiptune>(frameDuration, std::move(model));
  }

  Chiptune::Ptr CreateStreamedChiptune(StreamModel::Ptr model)
  {
    return CreateStreamedChiptune(BASE_FRAME_DURATION, std::move(model));
  }
}  // namespace Module::TFM
