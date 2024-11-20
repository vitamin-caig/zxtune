/**
 *
 * @file
 *
 * @brief  AYM-based stream chiptunes support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/players/aym/aym_base_stream.h"

#include "module/players/streaming.h"

#include "make_ptr.h"

#include <utility>

namespace Module::AYM
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

    Devices::AYM::Registers GetData() const override
    {
      return Data->Get(Delegate->CurrentFrame());
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

    TrackModel::Ptr FindTrackModel() const override
    {
      return {};
    }

    Module::StreamModel::Ptr FindStreamModel() const override
    {
      return Data;
    }

    Parameters::Accessor::Ptr GetProperties() const override
    {
      return Properties;
    }

    DataIterator::Ptr CreateDataIterator(TrackParameters::Ptr /*trackParams*/) const override
    {
      auto iter = CreateStreamStateIterator(FrameDuration, *Data);
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
}  // namespace Module::AYM
