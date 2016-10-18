/**
* 
* @file
*
* @brief  AYM-based stream chiptunes support implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "aym_base_stream.h"
//common includes
#include <make_ptr.h>
//library includes
#include <module/players/streaming.h>
//std includes
#include <utility>

namespace Module
{
  namespace AYM
  {
    class StreamDataIterator : public DataIterator
    {
    public:
      StreamDataIterator(StateIterator::Ptr delegate, StreamModel::Ptr data)
        : Delegate(std::move(delegate))
        , State(Delegate->GetStateObserver())
        , Data(std::move(data))
      {
      }

      void Reset() override
      {
        Delegate->Reset();
      }

      bool IsValid() const override
      {
        return Delegate->IsValid();
      }

      void NextFrame(bool looped) override
      {
        Delegate->NextFrame(looped);
      }

      TrackState::Ptr GetStateObserver() const override
      {
        return State;
      }

      Devices::AYM::Registers GetData() const override
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
        : Data(std::move(model))
        , Properties(std::move(properties))
        , Info(CreateStreamInfo(Data->Size(), Data->Loop()))
      {
      }

      Information::Ptr GetInformation() const override
      {
        return Info;
      }

      Parameters::Accessor::Ptr GetProperties() const override
      {
        return Properties;
      }

      DataIterator::Ptr CreateDataIterator(TrackParameters::Ptr /*trackParams*/) const override
      {
        const StateIterator::Ptr iter = CreateStreamStateIterator(Info);
        return MakePtr<StreamDataIterator>(iter, Data);
      }
    private:
      const StreamModel::Ptr Data;
      const Parameters::Accessor::Ptr Properties;
      const Information::Ptr Info;
    };

    Chiptune::Ptr CreateStreamedChiptune(StreamModel::Ptr model, Parameters::Accessor::Ptr properties)
    {
      return MakePtr<StreamedChiptune>(model, properties);
    }
  }
}
