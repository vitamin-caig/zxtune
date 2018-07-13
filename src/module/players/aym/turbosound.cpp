/**
* 
* @file
*
* @brief  TurboSound-based chiptunes support
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "turbosound.h"
//common includes
#include <error.h>
#include <iterator.h>
#include <make_ptr.h>
//library includes
#include <module/attributes.h>
#include <module/players/analyzer.h>
#include <parameters/merged_accessor.h>
#include <parameters/tracking_helper.h>
#include <parameters/visitor.h>
#include <sound/mixer_factory.h>
//std includes
#include <map>
#include <set>

namespace Module
{
namespace TurboSound
{
  class MergedModuleProperties : public Parameters::Accessor
  {
    static void MergeStringProperty(const Parameters::NameType& /*propName*/, String& lh, const String& rh)
    {
      if (lh != rh)
      {
        lh += '/';
        lh += rh;
      }
    }

    class MergedStringsVisitor : public Parameters::Visitor
    {
    public:
      explicit MergedStringsVisitor(Visitor& delegate)
        : Delegate(delegate)
      {
      }

      void SetValue(const Parameters::NameType& name, Parameters::IntType val) override
      {
        if (DoneIntegers.insert(name).second)
        {
          return Delegate.SetValue(name, val);
        }
      }

      void SetValue(const Parameters::NameType& name, const Parameters::StringType& val) override
      {
        const StringsValuesMap::iterator it = Strings.find(name);
        if (it == Strings.end())
        {
          Strings.insert(StringsValuesMap::value_type(name, val));
        }
        else
        {
          MergeStringProperty(name, it->second, val);
        }
      }

      void SetValue(const Parameters::NameType& name, const Parameters::DataType& val) override
      {
        if (DoneDatas.insert(name).second)
        {
          return Delegate.SetValue(name, val);
        }
      }

      void ProcessRestStrings() const
      {
        for (const auto& str : Strings)
        {
          Delegate.SetValue(str.first, str.second);
        }
      }
    private:
      Parameters::Visitor& Delegate;
      typedef std::map<Parameters::NameType, Parameters::StringType> StringsValuesMap;
      StringsValuesMap Strings;
      std::set<Parameters::NameType> DoneIntegers;
      std::set<Parameters::NameType> DoneDatas;
    };
  public:
    MergedModuleProperties(Parameters::Accessor::Ptr first, Parameters::Accessor::Ptr second)
      : First(std::move(first))
      , Second(std::move(second))
    {
    }

    uint_t Version() const override
    {
      return 1;
    }

    bool FindValue(const Parameters::NameType& name, Parameters::IntType& val) const override
    {
      return First->FindValue(name, val) || Second->FindValue(name, val);
    }

    bool FindValue(const Parameters::NameType& name, Parameters::StringType& val) const override
    {
      String val1, val2;
      const bool res1 = First->FindValue(name, val1);
      const bool res2 = Second->FindValue(name, val2);
      if (res1 && res2)
      {
        MergeStringProperty(name, val1, val2);
        val = val1;
      }
      else if (res1 != res2)
      {
        val = res1 ? val1 : val2;
      }
      return res1 || res2;
    }

    bool FindValue(const Parameters::NameType& name, Parameters::DataType& val) const override
    {
      return First->FindValue(name, val) || Second->FindValue(name, val);
    }

    void Process(Parameters::Visitor& visitor) const override
    {
      MergedStringsVisitor mergedVisitor(visitor);
      First->Process(mergedVisitor);
      Second->Process(mergedVisitor);
      mergedVisitor.ProcessRestStrings();
    }
  private:
    const Parameters::Accessor::Ptr First;
    const Parameters::Accessor::Ptr Second;
  };

  class MergedModuleInfo : public Information
  {
  public:
    MergedModuleInfo(Information::Ptr lh, Information::Ptr rh)
      : First(std::move(lh))
      , Second(std::move(rh))
    {
    }
    uint_t PositionsCount() const override
    {
      return First->PositionsCount();
    }
    uint_t LoopPosition() const override
    {
      return First->LoopPosition();
    }
    uint_t FramesCount() const override
    {
      return First->FramesCount();
    }
    uint_t LoopFrame() const override
    {
      return First->LoopFrame();
    }
    uint_t ChannelsCount() const override
    {
      return First->ChannelsCount() + Second->ChannelsCount();
    }
    uint_t Tempo() const override
    {
      return std::min(First->Tempo(), Second->Tempo());
    }
  private:
    const Information::Ptr First;
    const Information::Ptr Second;
  };

  class MergedTrackState : public TrackState
  {
  public:
    MergedTrackState(TrackState::Ptr first, TrackState::Ptr second)
      : First(std::move(first)), Second(std::move(second))
    {
    }

    uint_t Position() const override
    {
      return First->Position();
    }

    uint_t Pattern() const override
    {
      return First->Pattern();
    }

    uint_t PatternSize() const override
    {
      return First->PatternSize();
    }

    uint_t Line() const override
    {
      return First->Line();
    }

    uint_t Tempo() const override
    {
      return First->Tempo();
    }

    uint_t Quirk() const override
    {
      return First->Quirk();
    }

    uint_t Frame() const override
    {
      return First->Frame();
    }

    uint_t Channels() const override
    {
      return First->Channels() + Second->Channels();
    }
  private:
    const TrackState::Ptr First;
    const TrackState::Ptr Second;
  };

  class MergedDataIterator : public DataIterator
  {
  public:
    MergedDataIterator(AYM::DataIterator::Ptr first, AYM::DataIterator::Ptr second)
      : Observer(MakePtr<MergedTrackState>(first->GetStateObserver(), second->GetStateObserver()))
      , First(first)
      , Second(second)
    {
    }

    void Reset() override
    {
      First->Reset();
      Second->Reset();
    }

    bool IsValid() const override
    {
      return First->IsValid() && Second->IsValid();
    }

    void NextFrame(bool looped) override
    {
      First->NextFrame(looped);
      Second->NextFrame(true);
    }

    TrackState::Ptr GetStateObserver() const override
    {
      return Observer;
    }

    Devices::TurboSound::Registers GetData() const override
    {
      const Devices::TurboSound::Registers res = {{First->GetData(), Second->GetData()}};
      return res;
    }
  private:
    const TrackState::Ptr Observer;
    const AYM::DataIterator::Ptr First;
    const AYM::DataIterator::Ptr Second;
  };

  //parameters helper has no default ctor, so cannot be stored in array
  class ParametersHelpersSet
  {
  public:
    explicit ParametersHelpersSet(const TrackParametersArray& trackParams)
      : Delegate0(trackParams[0])
      , Delegate1(trackParams[1])
    {
      static_assert(std::tuple_size<TrackParametersArray>::value == 2, "Invalid layout");
    }

    const Parameters::TrackingHelper<AYM::TrackParameters>& operator[] (std::size_t idx) const
    {
      return idx == 0 ? Delegate0 : Delegate1;
    }
    
    Parameters::TrackingHelper<AYM::TrackParameters>& operator[] (std::size_t idx)
    {
      return idx == 0 ? Delegate0 : Delegate1;
    }
  private:
    Parameters::TrackingHelper<AYM::TrackParameters> Delegate0;
    Parameters::TrackingHelper<AYM::TrackParameters> Delegate1;
  };

  class DoubleDataIterator : public DataIterator
  {
  public:
    DoubleDataIterator(const TrackParametersArray& trackParams, TrackStateIterator::Ptr iterator, DataRenderersArray renderers)
      : Delegate(std::move(iterator))
      , State(Delegate->GetStateObserver())
      , Renderers(std::move(renderers))
      , Params(trackParams)
    {
    }

    void Reset() override
    {
      Delegate->Reset();
      for (uint_t idx = 0; idx != Devices::TurboSound::CHIPS; ++idx)
      {
        Params[idx].Reset();
        Renderers[idx]->Reset();
      }
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

    Devices::TurboSound::Registers GetData() const override
    {
      return Delegate->IsValid()
        ? GetCurrentChunk()
        : Devices::TurboSound::Registers();
    }
  private:
    Devices::TurboSound::Registers GetCurrentChunk() const
    {
      const Devices::TurboSound::Registers res = {{GetCurrentChunk(0), GetCurrentChunk(1)}};
      return res;
    }

    Devices::AYM::Registers GetCurrentChunk(uint_t idx) const
    {
      SynchronizeParameters(idx);
      AYM::TrackBuilder builder(Table[idx]);
      Renderers[idx]->SynthesizeData(*State, builder);
      return builder.GetResult();
    }

    void SynchronizeParameters(uint_t idx) const
    {
      if (Params[idx].IsChanged())
      {
        Params[idx]->FreqTable(Table[idx]);
      }
    }
  private:
    const TrackStateIterator::Ptr Delegate;
    const TrackModelState::Ptr State;
    const DataRenderersArray Renderers;
    ParametersHelpersSet Params;
    mutable FrequencyTable Table[Devices::TurboSound::CHIPS];
  };

  class Renderer : public Module::Renderer
  {
  public:
    Renderer(Sound::RenderParameters::Ptr params, DataIterator::Ptr iterator, Devices::TurboSound::Device::Ptr device)
      : Params(params)
      , Iterator(std::move(iterator))
      , Device(std::move(device))
      , FrameDuration()
      , Looped()
    {
#ifndef NDEBUG
//perform self-test
      for (; Iterator->IsValid(); Iterator->NextFrame(false));
      Iterator->Reset();
#endif
    }

    TrackState::Ptr GetTrackState() const override
    {
      return Iterator->GetStateObserver();
    }

    Analyzer::Ptr GetAnalyzer() const override
    {
      return TurboSound::CreateAnalyzer(Device);
    }

    bool RenderFrame() override
    {
      if (Iterator->IsValid())
      {
        SynchronizeParameters();
        if (LastChunk.TimeStamp == Devices::TurboSound::Stamp())
        {
          //first chunk
          TransferChunk();
        }
        Iterator->NextFrame(Looped);
        LastChunk.TimeStamp += FrameDuration;
        TransferChunk();
      }
      return Iterator->IsValid();
    }

    void Reset() override
    {
      Params.Reset();
      Iterator->Reset();
      Device->Reset();
      LastChunk.TimeStamp = Devices::TurboSound::Stamp();
      FrameDuration = Devices::TurboSound::Stamp();
      Looped = false;
    }

    void SetPosition(uint_t frameNum) override
    {
      const TrackState::Ptr state = Iterator->GetStateObserver();
      uint_t curFrame = state->Frame();
      if (curFrame > frameNum)
      {
        Iterator->Reset();
        Device->Reset();
        LastChunk.TimeStamp = Devices::TurboSound::Stamp();
        curFrame = 0;
      }
      while (curFrame < frameNum && Iterator->IsValid())
      {
        TransferChunk();
        Iterator->NextFrame(true);
        ++curFrame;
      }
    }
  private:
    void SynchronizeParameters()
    {
      if (Params.IsChanged())
      {
        FrameDuration = Params->FrameDuration();
        Looped = Params->Looped();
      }
    }

    void TransferChunk()
    {
      LastChunk.Data = Iterator->GetData();
      Device->RenderData(LastChunk);
    }
  private:
    Parameters::TrackingHelper<Sound::RenderParameters> Params;
    const TurboSound::DataIterator::Ptr Iterator;
    const Devices::TurboSound::Device::Ptr Device;
    Devices::TurboSound::DataChunk LastChunk;
    Devices::TurboSound::Stamp FrameDuration;
    bool Looped;
  };

  class MergedChiptune : public Chiptune
  {
  public:
    MergedChiptune(Parameters::Accessor::Ptr props, AYM::Chiptune::Ptr first, AYM::Chiptune::Ptr second)
      : Properties(std::move(props))
      , First(std::move(first))
      , Second(std::move(second))
    {
    }

    Information::Ptr GetInformation() const override
    {
      return MakePtr<MergedModuleInfo>(First->GetInformation(), Second->GetInformation());
    }

    Parameters::Accessor::Ptr GetProperties() const override
    {
      const Parameters::Accessor::Ptr mixProps = MakePtr<MergedModuleProperties>(First->GetProperties(), Second->GetProperties());
      return Parameters::CreateMergedAccessor(Properties, mixProps);
    }

    DataIterator::Ptr CreateDataIterator(const TrackParametersArray& trackParams) const override
    {
      const AYM::DataIterator::Ptr first = First->CreateDataIterator(trackParams[0]);
      const AYM::DataIterator::Ptr second = Second->CreateDataIterator(trackParams[1]);
      return MakePtr<MergedDataIterator>(first, second);
    }
  private:
    const Parameters::Accessor::Ptr Properties;
    const AYM::Chiptune::Ptr First;
    const AYM::Chiptune::Ptr Second;
  };

  class Holder : public Module::Holder
  {
  public:
    explicit Holder(Chiptune::Ptr chiptune)
      : Tune(std::move(chiptune))
    {
    }

    Information::Ptr GetModuleInformation() const override
    {
      return Tune->GetInformation();
    }

    Parameters::Accessor::Ptr GetModuleProperties() const override
    {
      return Tune->GetProperties();
    }

    Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target) const override
    {
      return TurboSound::CreateRenderer(*Tune, params, target);
    }
  private:
    const Chiptune::Ptr Tune;
  };

  Devices::TurboSound::Chip::Ptr CreateChip(Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target)
  {
    typedef Sound::ThreeChannelsMatrixMixer MixerType;
    const MixerType::Ptr mixer = MixerType::Create();
    const Parameters::Accessor::Ptr pollParams = Sound::CreateMixerNotificationParameters(params, mixer);
    const Devices::TurboSound::ChipParameters::Ptr chipParams = AYM::CreateChipParameters(pollParams);
    return Devices::TurboSound::CreateChip(chipParams, mixer, target);
  }

  DataIterator::Ptr CreateDataIterator(const TrackParametersArray& trackParams, TrackStateIterator::Ptr iterator,
      const DataRenderersArray& renderers)
  {
    return MakePtr<DoubleDataIterator>(trackParams, iterator, renderers);
  }

  Analyzer::Ptr CreateAnalyzer(Devices::TurboSound::Device::Ptr device)
  {
    if (Devices::StateSource::Ptr src = std::dynamic_pointer_cast<Devices::StateSource>(device))
    {
      return Module::CreateAnalyzer(src);
    }
    return Analyzer::Ptr();
  }

  Chiptune::Ptr CreateChiptune(Parameters::Accessor::Ptr params, AYM::Chiptune::Ptr first, AYM::Chiptune::Ptr second)
  {
    if (first->GetInformation()->FramesCount() >= second->GetInformation()->FramesCount())
    {
      return MakePtr<MergedChiptune>(params, first, second);
    }
    else
    {
      return MakePtr<MergedChiptune>(params, second, first);
    }
  }

  Renderer::Ptr CreateRenderer(Sound::RenderParameters::Ptr params, DataIterator::Ptr iterator, Devices::TurboSound::Device::Ptr device)
  {
    return MakePtr<Renderer>(params, iterator, device);
  }

  Renderer::Ptr CreateRenderer(const Chiptune& chiptune, Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target)
  {
    const Sound::RenderParameters::Ptr sndParams = Sound::RenderParameters::Create(params);
    const TrackParametersArray trackParams = {{AYM::TrackParameters::Create(params, 0), AYM::TrackParameters::Create(params, 1)}};
    const DataIterator::Ptr iterator = chiptune.CreateDataIterator(trackParams);
    const Devices::TurboSound::Chip::Ptr chip = CreateChip(params, target);
    return CreateRenderer(sndParams, iterator, chip);
  }

  Holder::Ptr CreateHolder(Chiptune::Ptr chiptune)
  {
    return MakePtr<Holder>(chiptune);
  }
}
}
