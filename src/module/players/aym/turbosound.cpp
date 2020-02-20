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
#include "module/players/aym/turbosound.h"
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

  template<class Base>
  class MergedInformationBase : public Base
  {
  public:
    MergedInformationBase(const Information& lh, const Information& rh)
      : Frames(lh.FramesCount())
      , Loop(lh.LoopFrame())
      , Channels(lh.ChannelsCount() + rh.ChannelsCount())
    {
    }

    uint_t FramesCount() const override
    {
      return Frames;
    }

    uint_t LoopFrame() const override
    {
      return Loop;
    }

    uint_t ChannelsCount() const override
    {
      return Channels;
    }
  private:
    const uint_t Frames;
    const uint_t Loop;
    const uint_t Channels;
  };

  using MergedInformation = MergedInformationBase<Information>;

  class MergedTrackInformation : public MergedInformationBase<TrackInformation>
  {
  public:
    MergedTrackInformation(const TrackInformation& lh, const TrackInformation& rh)
      : MergedInformationBase(lh, rh)
      , Positions(lh.PositionsCount())
      , LoopPos(lh.LoopPosition())
      , TempoValue(std::min(lh.Tempo(), rh.Tempo()))
    {
    }

    uint_t PositionsCount() const override
    {
      return Positions;
    }

    uint_t LoopPosition() const override
    {
      return LoopPos;
    }

    uint_t Tempo() const override
    {
      return TempoValue;
    }
  private:
    const uint_t Positions;
    const uint_t LoopPos;
    const uint_t TempoValue;
  };

  Information::Ptr CreateInformation(Information::Ptr lh, Information::Ptr rh)
  {
    const auto lhTrack = std::dynamic_pointer_cast<const TrackInformation>(lh);
    const auto rhTrack = std::dynamic_pointer_cast<const TrackInformation>(rh);
    if (lhTrack && rhTrack)
    {
      return MakePtr<MergedTrackInformation>(*lhTrack, *rhTrack);
    }
    else
    {
      return MakePtr<MergedInformation>(*lh, *rh);
    }
  }

  template<class Base>
  class MergedStateBase : public Base
  {
  public:
    MergedStateBase(typename Base::Ptr first, typename Base::Ptr second)
      : First(std::move(first)), Second(std::move(second))
    {
    }

    uint_t Frame() const override
    {
      return First->Frame();
    }

    uint_t LoopCount() const override
    {
      return First->LoopCount();
    }

  protected:
    const typename Base::Ptr First;
    const typename Base::Ptr Second;
  };

  using MergedState = MergedStateBase<State>;

  class MergedTrackState : public MergedStateBase<TrackState>
  {
  public:
    //required for msvs...
    MergedTrackState(Ptr lh, Ptr rh)
      : MergedStateBase(std::move(lh), std::move(rh))
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

    uint_t Channels() const override
    {
      return First->Channels() + Second->Channels();
    }
  };

  State::Ptr CreateState(State::Ptr lh, State::Ptr rh)
  {
    auto lhTrack = std::dynamic_pointer_cast<const TrackState>(lh);
    auto rhTrack = std::dynamic_pointer_cast<const TrackState>(rh);
    if (lhTrack && rhTrack)
    {
      return MakePtr<MergedTrackState>(std::move(lhTrack), std::move(rhTrack));
    }
    else
    {
      return MakePtr<MergedState>(std::move(lh), std::move(rh));
    }
  }

  class MergedDataIterator : public DataIterator
  {
  public:
    MergedDataIterator(AYM::DataIterator::Ptr first, AYM::DataIterator::Ptr second)
      : Observer(CreateState(first->GetStateObserver(), second->GetStateObserver()))
      , First(std::move(first))
      , Second(std::move(second))
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

    void NextFrame(const Sound::LoopParameters& looped) override
    {
      First->NextFrame(looped);
      Second->NextFrame({true, 0});
    }

    State::Ptr GetStateObserver() const override
    {
      return Observer;
    }

    Devices::TurboSound::Registers GetData() const override
    {
      return {{First->GetData(), Second->GetData()}};
    }
  private:
    const State::Ptr Observer;
    const AYM::DataIterator::Ptr First;
    const AYM::DataIterator::Ptr Second;
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
      for (; Iterator->IsValid(); Iterator->NextFrame({}));
      Iterator->Reset();
#endif
    }

    State::Ptr GetState() const override
    {
      return Iterator->GetStateObserver();
    }

    Analyzer::Ptr GetAnalyzer() const override
    {
      return TurboSound::CreateAnalyzer(Device);
    }

    bool RenderFrame() override
    {
      try
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
      catch (const std::exception&)
      {
        return false;
      }
    }

    void Reset() override
    {
      Params.Reset();
      Iterator->Reset();
      Device->Reset();
      LastChunk.TimeStamp = {};
      FrameDuration = {};
      Looped = {};
    }

    void SetPosition(uint_t frameNum) override
    {
      uint_t curFrame = GetState()->Frame();
      if (curFrame > frameNum)
      {
        Iterator->Reset();
        Device->Reset();
        LastChunk.TimeStamp = {};
        curFrame = 0;
      }
      while (curFrame < frameNum && Iterator->IsValid())
      {
        TransferChunk();
        Iterator->NextFrame({});
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
    Time::Duration<Devices::TurboSound::TimeUnit> FrameDuration;
    Sound::LoopParameters Looped;
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
      return CreateInformation(First->GetInformation(), Second->GetInformation());
    }

    Parameters::Accessor::Ptr GetProperties() const override
    {
      auto mixProps = MakePtr<MergedModuleProperties>(First->GetProperties(), Second->GetProperties());
      return Parameters::CreateMergedAccessor(Properties, std::move(mixProps));
    }

    DataIterator::Ptr CreateDataIterator(AYM::TrackParameters::Ptr first, AYM::TrackParameters::Ptr second) const override
    {
      return MakePtr<MergedDataIterator>(
	First->CreateDataIterator(std::move(first)),
        Second->CreateDataIterator(std::move(second))
      );
    }
  private:
    const Parameters::Accessor::Ptr Properties;
    const AYM::Chiptune::Ptr First;
    const AYM::Chiptune::Ptr Second;
  };

  Devices::TurboSound::Chip::Ptr CreateChip(Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target)
  {
    typedef Sound::ThreeChannelsMatrixMixer MixerType;
    auto mixer = MixerType::Create();
    auto pollParams = Sound::CreateMixerNotificationParameters(std::move(params), mixer);
    auto chipParams = AYM::CreateChipParameters(std::move(pollParams));
    return Devices::TurboSound::CreateChip(std::move(chipParams), std::move(mixer), std::move(target));
  }

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
      auto sndParams = Sound::RenderParameters::Create(params);
      auto iterator = Tune->CreateDataIterator(AYM::TrackParameters::Create(params, 0), AYM::TrackParameters::Create(params, 1));
      auto chip = CreateChip(std::move(params), std::move(target));
      return MakePtr<Renderer>(std::move(sndParams), std::move(iterator), std::move(chip));
    }
  private:
    const Chiptune::Ptr Tune;
  };

  Analyzer::Ptr CreateAnalyzer(Devices::TurboSound::Device::Ptr device)
  {
    if (auto src = std::dynamic_pointer_cast<Devices::StateSource>(device))
    {
      return Module::CreateAnalyzer(std::move(src));
    }
    return Analyzer::Ptr();
  }

  Chiptune::Ptr CreateChiptune(Parameters::Accessor::Ptr params, AYM::Chiptune::Ptr first, AYM::Chiptune::Ptr second)
  {
    if (first->GetInformation()->FramesCount() < second->GetInformation()->FramesCount())
    {
      std::swap(first, second);
    }
    return MakePtr<MergedChiptune>(std::move(params), std::move(first), std::move(second));
  }

  Holder::Ptr CreateHolder(Chiptune::Ptr chiptune)
  {
    return MakePtr<Holder>(std::move(chiptune));
  }
}
}
