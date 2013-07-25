/*
Abstract:
  TurboSound functionality helpers implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "ts_base.h"
#include "core/plugins/players/analyzer.h"
//common includes
#include <error.h>
#include <iterator.h>
//library includes
#include <core/module_attrs.h>
#include <devices/details/parameters_helper.h>
#include <sound/mixer_factory.h>
//std includes
#include <set>
//boost includes
#include <boost/make_shared.hpp>

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

      virtual void SetValue(const Parameters::NameType& name, Parameters::IntType val)
      {
        if (DoneIntegers.insert(name).second)
        {
          return Delegate.SetValue(name, val);
        }
      }

      virtual void SetValue(const Parameters::NameType& name, const Parameters::StringType& val)
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

      virtual void SetValue(const Parameters::NameType& name, const Parameters::DataType& val)
      {
        if (DoneDatas.insert(name).second)
        {
          return Delegate.SetValue(name, val);
        }
      }

      void ProcessRestStrings() const
      {
        for (StringsValuesMap::const_iterator it = Strings.begin(), lim = Strings.end(); it != lim; ++it)
        {
          Delegate.SetValue(it->first, it->second);
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
      : First(first)
      , Second(second)
    {
    }

    virtual uint_t Version() const
    {
      return 1;
    }

    virtual bool FindValue(const Parameters::NameType& name, Parameters::IntType& val) const
    {
      return First->FindValue(name, val) || Second->FindValue(name, val);
    }

    virtual bool FindValue(const Parameters::NameType& name, Parameters::StringType& val) const
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

    virtual bool FindValue(const Parameters::NameType& name, Parameters::DataType& val) const
    {
      return First->FindValue(name, val) || Second->FindValue(name, val);
    }

    virtual void Process(Parameters::Visitor& visitor) const
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
      : First(lh)
      , Second(rh)
    {
    }
    virtual uint_t PositionsCount() const
    {
      return First->PositionsCount();
    }
    virtual uint_t LoopPosition() const
    {
      return First->LoopPosition();
    }
    virtual uint_t PatternsCount() const
    {
      return First->PatternsCount() + Second->PatternsCount();
    }
    virtual uint_t FramesCount() const
    {
      return First->FramesCount();
    }
    virtual uint_t LoopFrame() const
    {
      return First->LoopFrame();
    }
    virtual uint_t ChannelsCount() const
    {
      return First->ChannelsCount() + Second->ChannelsCount();
    }
    virtual uint_t Tempo() const
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
      : First(first), Second(second)
    {
    }

    virtual uint_t Position() const
    {
      return First->Position();
    }

    virtual uint_t Pattern() const
    {
      return First->Pattern();
    }

    virtual uint_t PatternSize() const
    {
      return First->PatternSize();
    }

    virtual uint_t Line() const
    {
      return First->Line();
    }

    virtual uint_t Tempo() const
    {
      return First->Tempo();
    }

    virtual uint_t Quirk() const
    {
      return First->Quirk();
    }

    virtual uint_t Frame() const
    {
      return First->Frame();
    }

    virtual uint_t Channels() const
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
      : Observer(boost::make_shared<MergedTrackState>(first->GetStateObserver(), second->GetStateObserver()))
      , First(first)
      , Second(second)
    {
    }

    virtual void Reset()
    {
      First->Reset();
      Second->Reset();
    }

    virtual bool IsValid() const
    {
      return First->IsValid() && Second->IsValid();
    }

    virtual void NextFrame(bool looped)
    {
      First->NextFrame(looped);
      Second->NextFrame(looped);
    }

    virtual TrackState::Ptr GetStateObserver() const
    {
      return Observer;
    }

    virtual Devices::TurboSound::Registers GetData() const
    {
      const Devices::TurboSound::Registers res = {{First->GetData(), Second->GetData()}};
      return res;
    }
  private:
    const TrackState::Ptr Observer;
    const AYM::DataIterator::Ptr First;
    const AYM::DataIterator::Ptr Second;
  };

  class DoubleDataIterator : public DataIterator
  {
  public:
    DoubleDataIterator(AYM::TrackParameters::Ptr trackParams, TrackStateIterator::Ptr delegate, AYM::DataRenderer::Ptr first, AYM::DataRenderer::Ptr second)
      : Params(trackParams)
      , Delegate(delegate)
      , State(Delegate->GetStateObserver())
      , First(first)
      , Second(second)
    {
      FillCurrentChunk();
    }

    virtual void Reset()
    {
      Params.Reset();
      Delegate->Reset();
      First->Reset();
      Second->Reset();
      FillCurrentChunk();
    }

    virtual bool IsValid() const
    {
      return Delegate->IsValid();
    }

    virtual void NextFrame(bool looped)
    {
      Delegate->NextFrame(looped);
      FillCurrentChunk();
    }

    virtual TrackState::Ptr GetStateObserver() const
    {
      return State;
    }

    virtual Devices::TurboSound::Registers GetData() const
    {
      return CurrentData;
    }
  private:
    void FillCurrentChunk()
    {
      if (Delegate->IsValid())
      {
        SynchronizeParameters();
        {
          AYM::TrackBuilder builder(Table);
          First->SynthesizeData(*State, builder);
          CurrentData[0] = builder.GetResult();
        }
        {
          AYM::TrackBuilder builder(Table);
          Second->SynthesizeData(*State, builder);
          CurrentData[1] = builder.GetResult();
        }
      }
    }

    void SynchronizeParameters()
    {
      if (Params.IsChanged())
      {
        Params->FreqTable(Table);
      }
    }
  private:
    Devices::Details::ParametersHelper<AYM::TrackParameters> Params;
    const TrackStateIterator::Ptr Delegate;
    const TrackModelState::Ptr State;
    const AYM::DataRenderer::Ptr First;
    const AYM::DataRenderer::Ptr Second;
    Devices::TurboSound::Registers CurrentData;
    FrequencyTable Table;
  };

  class Renderer : public Module::Renderer
  {
  public:
    Renderer(Sound::RenderParameters::Ptr params, DataIterator::Ptr iterator, Devices::TurboSound::Device::Ptr device)
      : Params(params)
      , Iterator(iterator)
      , Device(device)
      , FrameDuration()
      , Looped()
    {
#ifndef NDEBUG
//perform self-test
      for (; Iterator->IsValid(); Iterator->NextFrame(false));
      Iterator->Reset();
#endif
    }

    virtual TrackState::Ptr GetTrackState() const
    {
      return Iterator->GetStateObserver();
    }

    virtual Analyzer::Ptr GetAnalyzer() const
    {
      return TurboSound::CreateAnalyzer(Device);
    }

    virtual bool RenderFrame()
    {
      if (Iterator->IsValid())
      {
        SynchronizeParameters();
        Devices::TurboSound::DataChunk chunk;
        chunk.TimeStamp = FlushChunk.TimeStamp;
        chunk.Data = Iterator->GetData();
        CommitChunk(chunk);
        Iterator->NextFrame(Looped);
        return Iterator->IsValid();
      }
      return false;
    }

    virtual void Reset()
    {
      Params.Reset();
      Iterator->Reset();
      Device->Reset();
      FlushChunk = Devices::TurboSound::DataChunk();
      FrameDuration = Devices::TurboSound::Stamp();
      Looped = false;
    }

    virtual void SetPosition(uint_t frameNum)
    {
      SeekIterator(*Iterator, frameNum);
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

    void CommitChunk(const Devices::TurboSound::DataChunk& chunk)
    {
      Device->RenderData(chunk);
      FlushChunk.TimeStamp += FrameDuration;
      Device->RenderData(FlushChunk);
      Device->Flush();
    }
  private:
    Devices::Details::ParametersHelper<Sound::RenderParameters> Params;
    const DataIterator::Ptr Iterator;
    const Devices::TurboSound::Device::Ptr Device;
    Devices::TurboSound::DataChunk FlushChunk;
    Devices::TurboSound::Stamp FrameDuration;
    bool Looped;
  };

  class MergedChiptune : public Chiptune
  {
  public:
    MergedChiptune(Parameters::Accessor::Ptr props, AYM::Chiptune::Ptr first, AYM::Chiptune::Ptr second)
      : Properties(props)
      , First(first)
      , Second(second)
    {
    }

    virtual Information::Ptr GetInformation() const
    {
      return boost::make_shared<MergedModuleInfo>(First->GetInformation(), Second->GetInformation());
    }

    virtual Parameters::Accessor::Ptr GetProperties() const
    {
      const Parameters::Accessor::Ptr mixProps = boost::make_shared<MergedModuleProperties>(First->GetProperties(), Second->GetProperties());
      return Parameters::CreateMergedAccessor(Properties, mixProps);
    }

    virtual TurboSound::DataIterator::Ptr CreateDataIterator(AYM::TrackParameters::Ptr trackParams) const
    {
      const AYM::DataIterator::Ptr first = First->CreateDataIterator(trackParams);
      const AYM::DataIterator::Ptr second = Second->CreateDataIterator(trackParams);
      return boost::make_shared<MergedDataIterator>(first, second);
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
      : Tune(chiptune)
    {
    }

    virtual Information::Ptr GetModuleInformation() const
    {
      return Tune->GetInformation();
    }

    virtual Parameters::Accessor::Ptr GetModuleProperties() const
    {
      return Tune->GetProperties();
    }

    virtual Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target) const
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

  DataIterator::Ptr CreateDataIterator(AYM::TrackParameters::Ptr trackParams, TrackStateIterator::Ptr iterator,
    AYM::DataRenderer::Ptr first, AYM::DataRenderer::Ptr second)
  {
    return boost::make_shared<DoubleDataIterator>(trackParams, iterator, first, second);
  }

  Analyzer::Ptr CreateAnalyzer(Devices::TurboSound::Device::Ptr device)
  {
    if (Devices::StateSource::Ptr src = boost::dynamic_pointer_cast<Devices::StateSource>(device))
    {
      return Module::CreateAnalyzer(src);
    }
    return Analyzer::Ptr();
  }

  Chiptune::Ptr CreateChiptune(Parameters::Accessor::Ptr params, AYM::Chiptune::Ptr first, AYM::Chiptune::Ptr second)
  {
    return boost::make_shared<MergedChiptune>(params, first, second);
  }

  Renderer::Ptr CreateRenderer(Sound::RenderParameters::Ptr params, DataIterator::Ptr iterator, Devices::TurboSound::Device::Ptr device)
  {
    return boost::make_shared<Renderer>(params, iterator, device);
  }

  Renderer::Ptr CreateRenderer(const Chiptune& chiptune, Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target)
  {
    const Sound::RenderParameters::Ptr sndParams = Sound::RenderParameters::Create(params);
    const AYM::TrackParameters::Ptr trackParams = AYM::TrackParameters::Create(params);
    const DataIterator::Ptr iterator = chiptune.CreateDataIterator(trackParams);
    const Devices::TurboSound::Chip::Ptr chip = CreateChip(params, target);
    return CreateRenderer(sndParams, iterator, chip);
  }

  Holder::Ptr CreateHolder(Chiptune::Ptr chiptune)
  {
    return boost::make_shared<Holder>(chiptune);
  }
}
}
