/**
* 
* @file
*
* @brief  TFM-based stream chiptunes support implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "tfm_base_stream.h"
//common includes
#include <make_ptr.h>
#include <module/players/streaming.h>

namespace Module
{
  namespace TFM
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

      void GetData(Devices::TFM::Registers& res) const override
      {
        if (Delegate->IsValid())
        {
          Data->Get(State->Frame(), res);
        }
        else
        {
          res.clear();
        }
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

      Information::Ptr GetInformation() const override
      {
        return Info;
      }

      Parameters::Accessor::Ptr GetProperties() const override
      {
        return Properties;
      }

      TFM::DataIterator::Ptr CreateDataIterator() const override
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
