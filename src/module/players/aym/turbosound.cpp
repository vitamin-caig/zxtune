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

#include "module/players/parameters.h"
#include "module/players/streaming.h"
#include "parameters/src/names_set.h"

#include "module/attributes.h"
#include "parameters/merged_accessor.h"
#include "parameters/visitor.h"
#include "sound/mixer_factory.h"
#include "tools/iterators.h"

#include "error.h"
#include "make_ptr.h"
#include "string_view.h"

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

  class MergedSource : public DataIterator
  {
  public:
    MergedSource(AYM::DataIterator::Ptr first, AYM::DataIterator::Ptr second)
      : First(std::move(first))
      , Second(std::move(second))
    {}

    FramedState GetState() const override
    {
      auto first = First->GetState();
      auto second = Second->GetState();
      if (first.Track && second.Track)
      {
        first.Track->Channels += second.Track->Channels;
      }
      return first;
    }

    Devices::TurboSound::Registers Advance() override
    {
      auto first = First->Advance();
      auto second = Second->Advance();
      return {{std::move(first), std::move(second)}};
    }

    void Reset() override
    {
      First->Reset();
      Second->Reset();
    }

  private:
    const AYM::DataIterator::Ptr First;
    const AYM::DataIterator::Ptr Second;
  };

  class Renderer : public Module::Renderer
  {
  public:
    Renderer(FrameDurationParameter frameDuration, DataIterator::Ptr iterator, Devices::TurboSound::Chip::Ptr device)
      : FrameDuration(std::move(frameDuration))
      , Iterator(std::move(iterator))
      , Device(std::move(device))
    {}

    State GetState() const override
    {
      return ToState(Iterator->GetState(), FrameDuration.Get());
    }

    Sound::Chunk Render() override
    {
      TransferChunk();
      FrameDuration.Synchronize();
      LastChunk.TimeStamp += FrameDuration.Get<Devices::TurboSound::TimeUnit>();
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
      if (request < GetState().At)
      {
        Iterator->Reset();
        Device->Reset();
        LastChunk.TimeStamp = {};
      }
      while (GetState().At < request)
      {
        TransferChunk();
      }
    }

  private:
    void TransferChunk()
    {
      LastChunk.Data = Iterator->Advance();
      Device->RenderData(LastChunk);
    }

  private:
    FrameDurationParameter FrameDuration;
    const DataIterator::Ptr Iterator;
    const Devices::TurboSound::Chip::Ptr Device;
    Devices::TurboSound::DataChunk LastChunk;
  };

  class MergedChiptune : public Chiptune
  {
  public:
    MergedChiptune(AYM::Chiptune::Ptr first, AYM::Chiptune::Ptr second)
      : First(std::move(first))
      , Second(std::move(second))
    {}

    Time::Microseconds GetFrameDuration() const override
    {
      return First->GetFrameDuration();
    }

    FramedInformation GetInformation() const override
    {
      auto info = First->GetInformation();
      if (info.Track)
      {
        info.Track->ChannelsCount = TRACK_CHANNELS;
      }
      return info;
    }

    DataIterator::Ptr CreateDataIterator(AYM::TrackParameters::Ptr first,
                                         AYM::TrackParameters::Ptr second) const override
    {
      return MakePtr<MergedSource>(First->CreateDataIterator(std::move(first)),
                                   Second->CreateDataIterator(std::move(second)));
    }

  private:
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
    Holder(Chiptune::Ptr chiptune, Parameters::Accessor::Ptr properties, Time::Microseconds frameDuration)
      : Tune(std::move(chiptune))
      , Properties(std::move(properties))
      , FrameDuration(frameDuration)
    {}

    Information GetModuleInformation() const override
    {
      return ToInformation(Tune->GetInformation(), FrameDuration);
    }

    Parameters::Accessor::Ptr GetModuleProperties() const override
    {
      return Properties;
    }

    Renderer::Ptr CreateRenderer(uint_t samplerate, Parameters::Accessor::Ptr params) const override
    {
      auto rendererParams = params;
      auto iterator =
          Tune->CreateDataIterator(AYM::TrackParameters::Create(params, 0), AYM::TrackParameters::Create(params, 1));
      auto chip = CreateChip(samplerate, std::move(params));
      return MakePtr<Renderer>(FrameDurationParameter(std::move(rendererParams), AYM::BASE_FRAME_DURATION),
                               std::move(iterator), std::move(chip));
    }

  private:
    const Chiptune::Ptr Tune;
    const Parameters::Accessor::Ptr Properties;
    const Time::Microseconds FrameDuration;
  };

  Chiptune::Ptr CreateChiptune(AYM::Chiptune::Ptr first, AYM::Chiptune::Ptr second)
  {
    /* TODO: think about it
    if (first->GetInformation()->FramesCount() < second->GetInformation()->FramesCount())
    {
      std::swap(first, second);
    }
    */
    return MakePtr<MergedChiptune>(std::move(first), std::move(second));
  }

  Parameters::Accessor::Ptr CreateModuleProperties(Parameters::Accessor::Ptr outer, Parameters::Accessor::Ptr first,
                                                   Parameters::Accessor::Ptr second)
  {
    auto mixProps = MakePtr<MergedModuleProperties>(std::move(first), std::move(second));
    return Parameters::CreateMergedAccessor(std::move(outer), std::move(mixProps));
  }

  Holder::Ptr CreateHolder(Chiptune::Ptr chiptune, Parameters::Accessor::Ptr properties)
  {
    const FrameDurationParameter frameDuration(properties, AYM::BASE_FRAME_DURATION);
    return MakePtr<Holder>(std::move(chiptune), std::move(properties), frameDuration.Get());
  }
}  // namespace Module::TurboSound
