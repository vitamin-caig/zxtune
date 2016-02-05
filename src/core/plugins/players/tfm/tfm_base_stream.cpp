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
#include "core/plugins/players/streaming.h"
//common includes
#include <make_ptr.h>

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

      virtual void GetData(Devices::TFM::Registers& res) const
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

      virtual Information::Ptr GetInformation() const
      {
        return Info;
      }

      virtual Parameters::Accessor::Ptr GetProperties() const
      {
        return Properties;
      }

      virtual TFM::DataIterator::Ptr CreateDataIterator() const
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
