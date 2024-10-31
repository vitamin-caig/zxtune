/**
 *
 * @file
 *
 * @brief  TurboSound-based chiptunes support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/players/aym/turbosound.h"

#include "module/players/streaming.h"
#include <parameters/src/names_set.h>

#include <module/attributes.h>
#include <parameters/merged_accessor.h>
#include <parameters/visitor.h>
#include <sound/mixer_factory.h>
#include <tools/iterators.h>

#include <error.h>
#include <make_ptr.h>
#include <string_view.h>

#include <map>
#include <set>

namespace Module::TurboSound
{
  class MergedModuleProperties : public Parameters::Accessor
  {
    static void MergeStringProperty(StringView /*propName*/, String& lh, StringView rh)
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
      {}

      void SetValue(Parameters::Identifier name, Parameters::IntType val) override
      {
        if (DoneIntegers.Insert(name))
        {
          return Delegate.SetValue(name, val);
        }
      }

      void SetValue(Parameters::Identifier name, StringView val) override
      {
        const auto it = Strings.find(static_cast<StringView>(name));
        if (it == Strings.end())
        {
          Strings.emplace(name.AsString(), val);
        }
        else
        {
          MergeStringProperty(name, it->second, val);
        }
      }

      void SetValue(Parameters::Identifier name, Binary::View val) override
      {
        if (DoneDatas.Insert(name))
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
      using StringsValuesMap = std::map<String, Parameters::StringType, std::less<>>;
      StringsValuesMap Strings;
      Parameters::NamesSet DoneIntegers;
      Parameters::NamesSet DoneDatas;
    };

  public:
    MergedModuleProperties(Parameters::Accessor::Ptr first, Parameters::Accessor::Ptr second)
      : First(std::move(first))
      , Second(std::move(second))
    {}

    uint_t Version() const override
    {
      return 1;
    }

    std::optional<Parameters::IntType> FindInteger(Parameters::Identifier name) const override
    {
      if (auto first = First->FindInteger(name))
      {
        return first;
      }
      return Second->FindInteger(name);
    }

    std::optional<Parameters::StringType> FindString(Parameters::Identifier name) const override
    {
      auto val1 = First->FindString(name);
      auto val2 = Second->FindString(name);
      if (val1 && val2)
      {
        MergeStringProperty(name, *val1, *val2);
        return val1;
      }
      return val1 ? val1 : val2;
    }

    Binary::Data::Ptr FindData(Parameters::Identifier name) const override
    {
      if (auto first = First->FindData(name))
      {
        return first;
      }
      return Second->FindData(name);
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
  class MergedStateBase : public Base
  {
  public:
    MergedStateBase(typename Base::Ptr first, typename Base::Ptr second)
      : First(std::move(first))
      , Second(std::move(second))
    {}

    Time::AtMillisecond At() const override
    {
      return First->At();
    }

    Time::Milliseconds Total() const override
    {
      return First->Total();
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
    // required for msvs...
    MergedTrackState(Ptr lh, Ptr rh)
      : MergedStateBase(std::move(lh), std::move(rh))
    {}

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
    {}

    void Reset() override
    {
      First->Reset();
      Second->Reset();
    }

    void NextFrame() override
    {
      First->NextFrame();
      Second->NextFrame();
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
    Renderer(Time::Microseconds frameDuration, DataIterator::Ptr iterator, Devices::TurboSound::Chip::Ptr device)
      : Iterator(std::move(iterator))
      , Device(std::move(device))
      , FrameDuration(frameDuration)
    {}

    State::Ptr GetState() const override
    {
      return Iterator->GetStateObserver();
    }

    Sound::Chunk Render() override
    {
      TransferChunk();
      Iterator->NextFrame();
      LastChunk.TimeStamp += FrameDuration;
      return Device->RenderTill(LastChunk.TimeStamp);
    }

    void Reset() override
    {
      Iterator->Reset();
      Device->Reset();
      LastChunk.TimeStamp = {};
    }

    void SetPosition(Time::AtMillisecond request) override
    {
      const auto state = GetState();
      if (request < state->At())
      {
        Iterator->Reset();
        Device->Reset();
        LastChunk.TimeStamp = {};
      }
      while (state->At() < request)
      {
        TransferChunk();
        Iterator->NextFrame();
      }
    }

  private:
    void TransferChunk()
    {
      LastChunk.Data = Iterator->GetData();
      Device->RenderData(LastChunk);
    }

  private:
    const TurboSound::DataIterator::Ptr Iterator;
    const Devices::TurboSound::Chip::Ptr Device;
    const Time::Duration<Devices::TurboSound::TimeUnit> FrameDuration;
    Devices::TurboSound::DataChunk LastChunk;
  };

  class MergedChiptune : public Chiptune
  {
  public:
    MergedChiptune(Parameters::Accessor::Ptr props, AYM::Chiptune::Ptr first, AYM::Chiptune::Ptr second)
      : Properties(std::move(props))
      , First(std::move(first))
      , Second(std::move(second))
    {}

    Time::Microseconds GetFrameDuration() const override
    {
      return First->GetFrameDuration();
    }

    TrackModel::Ptr FindTrackModel() const override
    {
      return First->FindTrackModel();
    }

    Module::StreamModel::Ptr FindStreamModel() const override
    {
      return First->FindStreamModel();
    }

    Parameters::Accessor::Ptr GetProperties() const override
    {
      auto mixProps = MakePtr<MergedModuleProperties>(First->GetProperties(), Second->GetProperties());
      return Parameters::CreateMergedAccessor(Properties, std::move(mixProps));
    }

    DataIterator::Ptr CreateDataIterator(AYM::TrackParameters::Ptr first,
                                         AYM::TrackParameters::Ptr second) const override
    {
      return MakePtr<MergedDataIterator>(First->CreateDataIterator(std::move(first)),
                                         Second->CreateDataIterator(std::move(second)));
    }

  private:
    const Parameters::Accessor::Ptr Properties;
    const AYM::Chiptune::Ptr First;
    const AYM::Chiptune::Ptr Second;
  };

  Devices::TurboSound::Chip::Ptr CreateChip(uint_t samplerate, Parameters::Accessor::Ptr params)
  {
    using MixerType = Sound::ThreeChannelsMatrixMixer;
    auto mixer = MixerType::Create();
    auto pollParams = Sound::CreateMixerNotificationParameters(std::move(params), mixer);
    auto chipParams = AYM::CreateChipParameters(samplerate, std::move(pollParams));
    return Devices::TurboSound::CreateChip(std::move(chipParams), std::move(mixer));
  }

  class Holder : public Module::Holder
  {
  public:
    Holder(Chiptune::Ptr chiptune)
      : Tune(std::move(chiptune))
    {}

    Information::Ptr GetModuleInformation() const override
    {
      if (auto track = Tune->FindTrackModel())
      {
        return CreateTrackInfoFixedChannels(Tune->GetFrameDuration(), std::move(track), TRACK_CHANNELS);
      }
      else
      {
        return CreateStreamInfo(Tune->GetFrameDuration(), *Tune->FindStreamModel());
      }
    }

    Parameters::Accessor::Ptr GetModuleProperties() const override
    {
      return Tune->GetProperties();
    }

    Renderer::Ptr CreateRenderer(uint_t samplerate, Parameters::Accessor::Ptr params) const override
    {
      auto iterator =
          Tune->CreateDataIterator(AYM::TrackParameters::Create(params, 0), AYM::TrackParameters::Create(params, 1));
      auto chip = CreateChip(samplerate, std::move(params));
      return MakePtr<Renderer>(Tune->GetFrameDuration() /*TODO: speed variation*/, std::move(iterator),
                               std::move(chip));
    }

  private:
    const Chiptune::Ptr Tune;
  };

  Chiptune::Ptr CreateChiptune(Parameters::Accessor::Ptr params, AYM::Chiptune::Ptr first, AYM::Chiptune::Ptr second)
  {
    /* TODO: think about it
    if (first->GetInformation()->FramesCount() < second->GetInformation()->FramesCount())
    {
      std::swap(first, second);
    }
    */
    return MakePtr<MergedChiptune>(std::move(params), std::move(first), std::move(second));
  }

  Holder::Ptr CreateHolder(Chiptune::Ptr chiptune)
  {
    return MakePtr<Holder>(std::move(chiptune));
  }
}  // namespace Module::TurboSound
