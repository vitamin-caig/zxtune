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
#include "module/players/aym/aym_base_stream.h"
//common includes
#include <make_ptr.h>
//library includes
#include <module/players/streaming.h>
#include <sound/render_params.h>
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

      void NextFrame(const Sound::LoopParameters& looped) override
      {
        Delegate->NextFrame(looped);
      }

      Module::State::Ptr GetStateObserver() const override
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
        auto iter = CreateStreamStateIterator(MakeFramedStream());
        return MakePtr<StreamDataIterator>(std::move(iter), Data);
      }
    private:
      FramedStream MakeFramedStream() const
      {
        //TODO: remove
        FramedStream result;
        result.TotalFrames = Data->GetTotalFrames();
        result.LoopFrame = Data->GetLoopFrame();
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
