/*
Abstract:
  AYM-based stream players common functionality implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "aym_base_stream.h"
#include "core/plugins/players/streaming.h"

namespace Module
{
  namespace AYM
  {
    class StreamDataIterator : public DataIterator
    {
    public:
      StreamDataIterator(StateIterator::Ptr delegate, StreamModel::Ptr data)
        : Delegate(delegate)
        , State(Delegate->GetStateObserver())
        , Data(data)
      {
      }

      virtual void Reset()
      {
        Delegate->Reset();
      }

      virtual bool IsValid() const
      {
        return Delegate->IsValid();
      }

      virtual void NextFrame(bool looped)
      {
        Delegate->NextFrame(looped);
      }

      virtual TrackState::Ptr GetStateObserver() const
      {
        return State;
      }

      virtual Devices::AYM::Registers GetData() const
      {
        return Delegate->IsValid()
          ? Data->Get(State->Frame())
          : Devices::AYM::Registers();
      }
    private:
      const StateIterator::Ptr Delegate;
      const TrackState::Ptr State;
      const StreamModel::Ptr Data;
    };

    class StreamedChiptune : public Chiptune
    {
    public:
      StreamedChiptune(StreamModel::Ptr model, Parameters::Accessor::Ptr properties)
        : Data(model)
        , Properties(properties)
        , Info(CreateStreamInfo(Data->Size(), Data->Loop()))
      {
      }

      virtual Information::Ptr GetInformation() const
      {
        return Info;
      }

      virtual Parameters::Accessor::Ptr GetProperties() const
      {
        return Properties;
      }

      virtual DataIterator::Ptr CreateDataIterator(TrackParameters::Ptr /*trackParams*/) const
      {
        const StateIterator::Ptr iter = CreateStreamStateIterator(Info);
        return boost::make_shared<StreamDataIterator>(iter, Data);
      }
    private:
      const StreamModel::Ptr Data;
      const Parameters::Accessor::Ptr Properties;
      const Information::Ptr Info;
    };

    Chiptune::Ptr CreateStreamedChiptune(StreamModel::Ptr model, Parameters::Accessor::Ptr properties)
    {
      return boost::make_shared<StreamedChiptune>(model, properties);
    }
  }
}
