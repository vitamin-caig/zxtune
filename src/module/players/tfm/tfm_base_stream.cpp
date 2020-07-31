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
#include "module/players/tfm/tfm_base_stream.h"
//common includes
#include <make_ptr.h>
#include <module/players/streaming.h>
#include <sound/render_params.h>
//std includes
#include <utility>

namespace Module
{
  namespace TFM
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

      void NextFrame(const Sound::LoopParameters& looped) override
      {
        Delegate->NextFrame(looped);
      }

      Module::State::Ptr GetStateObserver() const override
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
      const Module::State::Ptr State;
      const StreamModel::Ptr Data;
    };

    class StreamedChiptune : public Chiptune
    {
    public:
      StreamedChiptune(StreamModel::Ptr model, Parameters::Accessor::Ptr properties)
        : Data(std::move(model))
        , Properties(std::move(properties))
      {
      }

      Information::Ptr GetInformation() const override
      {
        return CreateStreamInfo(MakeFramedStream());
      }

      Parameters::Accessor::Ptr GetProperties() const override
      {
        return Properties;
      }

      TFM::DataIterator::Ptr CreateDataIterator() const override
      {
        auto iter = CreateStreamStateIterator(MakeFramedStream());
        return MakePtr<StreamDataIterator>(std::move(iter), Data);
      }
    private:
      FramedStream MakeFramedStream() const
      {
        FramedStream result;
        result.TotalFrames = Data->Size();
        result.LoopFrame = Data->Loop();
        result.FrameDuration = Sound::GetFrameDuration(*Properties);
        return result;
      }
    private:
      const StreamModel::Ptr Data;
      const Parameters::Accessor::Ptr Properties;
    };

    Chiptune::Ptr CreateStreamedChiptune(StreamModel::Ptr model, Parameters::Accessor::Ptr properties)
    {
      return MakePtr<StreamedChiptune>(std::move(model), std::move(properties));
    }
  }
}
