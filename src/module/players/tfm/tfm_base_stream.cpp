/**
 *
 * @file
 *
 * @brief  TFM-based stream chiptunes support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "module/players/tfm/tfm_base_stream.h"
// common includes
#include <make_ptr.h>
#include <module/players/streaming.h>
// std includes
#include <utility>

namespace Module::TFM
{
  class StreamDataIterator : public DataIterator
  {
  public:
    StreamDataIterator(StateIterator::Ptr delegate, StreamModel::Ptr data)
      : Delegate(std::move(delegate))
      , State(Delegate->GetStateObserver())
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

    Module::State::Ptr GetStateObserver() const override
    {
      return State;
    }

    void GetData(Devices::TFM::Registers& res) const override
    {
      Data->Get(Delegate->CurrentFrame(), res);
    }

  private:
    const StateIterator::Ptr Delegate;
    const Module::State::Ptr State;
    const StreamModel::Ptr Data;
  };

  class StreamedChiptune : public Chiptune
  {
  public:
    StreamedChiptune(Time::Microseconds frameDuration, StreamModel::Ptr model, Parameters::Accessor::Ptr properties)
      : FrameDuration(frameDuration)
      , Data(std::move(model))
      , Properties(std::move(properties))
    {}

    Time::Microseconds GetFrameDuration() const override
    {
      return FrameDuration;
    }

    Information::Ptr GetInformation() const override
    {
      return CreateStreamInfo(FrameDuration, Data);
    }

    Parameters::Accessor::Ptr GetProperties() const override
    {
      return Properties;
    }

    TFM::DataIterator::Ptr CreateDataIterator() const override
    {
      auto iter = CreateStreamStateIterator(FrameDuration, Data);
      return MakePtr<StreamDataIterator>(std::move(iter), Data);
    }

  private:
    const Time::Microseconds FrameDuration;
    const StreamModel::Ptr Data;
    const Parameters::Accessor::Ptr Properties;
  };

  Chiptune::Ptr CreateStreamedChiptune(Time::Microseconds frameDuration, StreamModel::Ptr model,
                                       Parameters::Accessor::Ptr properties)
  {
    return MakePtr<StreamedChiptune>(frameDuration, std::move(model), std::move(properties));
  }
}  // namespace Module::TFM
