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

    Devices::AYM::Registers GetData() const override
    {
      return Data->Get(Delegate->GetState().At);
    }

  private:
    const Iterator::Ptr Delegate;
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

    FramedInformation GetInformation() const override
    {
      return Data->GetInformation();
    }

    Parameters::Accessor::Ptr GetProperties() const override
    {
      return Properties;
    }

    DataIterator::Ptr CreateDataIterator(TrackParameters::Ptr /*trackParams*/) const override
    {
      auto iter = CreateStreamStateIterator(Data->GetInformation());
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
